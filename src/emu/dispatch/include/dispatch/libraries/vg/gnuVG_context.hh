/*
 * gnuVG - a free Vector Graphics library
 * Copyright (C) 2014 by Anton Persson
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation, either version 3 of
 *  the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __GNUVG_CONTEXT_HH
#define __GNUVG_CONTEXT_HH

#include <dispatch/libraries/vg/consts.h>
#include <dispatch/libraries/vg/gnuVG_math.hh>
#include <dispatch/libraries/vg/gnuVG_idallocator.hh>
#include <dispatch/libraries/vg/gnuVG_object.hh>
#include <dispatch/libraries/vg/gnuVG_paint.hh>
#include <dispatch/libraries/vg/gnuVG_shader.hh>
#include <dispatch/libraries/vg/gnuVG_graphicstate.hh>
#include <dispatch/libraries/egl/def.h>
#include <dispatch/libraries/buffer_pusher.h>

#include <drivers/graphics/common.h>

#include <map>
#include <vector>
#include <stack>

#define GNUVG_MAX_SCISSORS 32

namespace gnuVG {
	class Context : public eka2l1::dispatch::egl_context {
	public:
		struct FrameBuffer {
			eka2l1::drivers::handle framebuffer = 0;
			VGint width = 128, height = 128;
			VGint subset_x = -1, subset_y = -1, subset_width = -1, subset_height = -1;
		};

		enum gnuVGFrameBuffer {
			GNUVG_CURRENT_FRAMEBUFFER,
			GNUVG_TEMPORARY_A,
			GNUVG_TEMPORARY_B,
			GNUVG_MASK_BUFFER
		};

		enum MatrixMode {
			GNUVG_MATRIX_PATH_USER_TO_SURFACE = 0,
			GNUVG_MATRIX_IMAGE_USER_TO_SURFACE = 1,
			GNUVG_MATRIX_FILL_PAINT_TO_USER = 2,
			GNUVG_MATRIX_STROKE_PAINT_TO_USER = 3,
			GNUVG_MATRIX_GLYPH_USER_TO_SURFACE = 4,
			GNUVG_MATRIX_MAX = 5
		};

		enum gnuVGPipeline {
			GNUVG_SIMPLE_PIPELINE
		};

	protected:
		std::map<VGHandle, std::shared_ptr<Object>> objects;

		gnuVGPipeline active_pipeline;
		VGPaintMode pipeline_mode;

		Shader::Blending blend_mode = Shader::blend_src_over;

		/* Last error generated by using the gnuVG API */
		VGErrorCode last_error;

		/* Matrices */
		MatrixMode user_matrix; // current matrix index (on which the user performs operations)
		MatrixMode conversion_matrix; // current matrix index (which we use for what we render now)
		Matrix matrix[GNUVG_MATRIX_MAX]; // the matrices representing the different OpenVG matrices
		Matrix screen_matrix; // screen_matrix maps from the "window" coordinates to the OpenGL space
		// this is the product of screen_matrix and matrix[current_matrix]
		Matrix final_matrix[GNUVG_MATRIX_MAX];
		bool matrix_is_dirty[GNUVG_MATRIX_MAX];
		float image_matrix_data[16]; // inverted IMAGE_USER_TO_SURFACE stored in 4d GL friendly matrix
		float conversion_matrix_data[16]; // final_matrix stored in 4d GL friendly matrix
		float surf2fill_matrix_data[16]; // surface2fill matrix stored in 4d GL friendly matrix
		float surf2stroke_matrix_data[16]; // surface2stroke matrix stored in 4d GL friendly matrix
		VGfloat pre_translation[2];

		/* Bounding box for all paths rendered since
		 * last reset of the bounding box
		 */
		Point bounding_box[2];
		bool bounding_box_was_reset = true; // indicate that the bounding box is empty

		/* Fonts and glyphs */
		VGfloat glyph_origin[2];

		/* Stroke data */
		std::vector<VGfloat> dash_pattern;
		VGfloat stroke_width, stroke_dash_phase;
		bool stroke_dash_phase_reset;
		VGfloat miter_limit;
		VGJoinStyle join_style;

		/* Paint info */
		Color clear_color;

		std::shared_ptr<Paint>  default_fill_paint, default_stroke_paint,
			fill_paint, stroke_paint;

		bool do_color_transform = false;
		VGfloat color_transform_scale[4];
		VGfloat color_transform_bias[4];

		void matrix_resize(VGint pixel_width, VGint pixel_height);

		// Buffer objects
		FrameBuffer screen_buffer;
		FrameBuffer mask;
		FrameBuffer temporary_a, temporary_b;
		VGint buffer_width, buffer_height;

		// Temporary framebuffers
		std::vector<FrameBuffer *> available_temporary_framebuffers;

		// Scissor data
		float scissor_vertices[GNUVG_MAX_SCISSORS * 4 * 2];
		std::uint32_t scissor_triangles[GNUVG_MAX_SCISSORS * 3 * 2];
		std::int32_t nr_active_scissors = 0; // the number of active scissors
		bool scissors_are_active = false;

		// Mask data
		bool mask_is_active = false;

		// Buffer
		eka2l1::dispatch::graphics_buffer_pusher vertex_buffer_pusher;
		eka2l1::dispatch::graphics_buffer_pusher index_buffer_pusher;

		eka2l1::drivers::handle input_desc = 0;

	protected: /* OpenGL specifics */
		/* Internal OpenGL state */
		Shader *active_shader;
		IDAllocator<VGHandle> idalloc;

		const FrameBuffer* current_framebuffer = nullptr;
		std::stack<const FrameBuffer*> framebuffer_storage;

		void render_scissors(const GraphicState &state);
		void recreate_buffers(const GraphicState &state);

	public:
		Context();
		~Context();

		eka2l1::dispatch::egl_context_type context_type() const override {
			return eka2l1::dispatch::EGL_VG_CONTEXT;
		}

		void register_object(std::shared_ptr<Object> object);
		void forget_object(VGHandle o_handle);
		std::shared_ptr<Object> get_object(VGHandle o_handle);

		void set_error(VGErrorCode new_error);
		VGErrorCode get_error();

		/* Backend Implementation specific (OpenGLES) */
		void resize(const GraphicState &state, VGint pixel_width, VGint pixel_height);
		void clear(const GraphicState &state, VGint x, VGint y, VGint width, VGint height);
		void trivial_render_elements(
			const GraphicState &state, float *vertices, std::int32_t vertices_count,
			std::uint32_t *indices, std::int32_t indices_count, VGfloat r, VGfloat g, VGfloat b, VGfloat a);
		void render_texture_alpha_triangle_array(const GraphicState &state,
							 const FrameBuffer *fb,
							 const float *ver_c_2d, const std::uint32_t ver_c_count, std::int32_t ver_stride_2d,
							 const float *tex_c_2d, const std::uint32_t tex_c_count, std::int32_t tex_stride_2d,
							 const std::uint32_t *indices, std::int32_t nr_indices,
							 const float *texture_matrix_3by3);
		void trivial_fill_area(const GraphicState &state,
			VGint x, VGint y, VGint width, VGint height,
			VGfloat r, VGfloat g, VGfloat b, VGfloat a);
		// if gaussian_width/height is <= 1 - no gaussian is done
		void trivial_render_framebuffer(const GraphicState &state,
						const FrameBuffer* framebuffer,
						int gaussian_width = 1,
						int gaussian_height = 1,
						VGTilingMode = VG_TILE_FILL);
		void prepare_framebuffer_matrix(const FrameBuffer* frbuf);
		void select_conversion_matrix(MatrixMode conversion_matrix);
		void use_pipeline(const GraphicState &state, gnuVGPipeline new_pipeline, VGPaintMode new_mode);
		void reset_pre_translation();
		void use_glyph_origin_as_pre_translation(VGfloat specific_glyph_origin[2]);
		void adjust_glyph_origin(VGfloat escapement[2]);
		void render_triangles(std::int32_t first, std::int32_t vertice_count);
		void render_elements(const GraphicState &state, const std::uint32_t *indices, std::int32_t nr_indices);
		void calculate_bounding_box(Point* bounding_box);
		void transform_bounding_box(Point* bbox, VGfloat *sp_ep);

		void get_pixelsize(VGint& width, VGint& height);
		void switch_mask_to(gnuVGFrameBuffer to_this_temporary);

		bool create_framebuffer(const GraphicState &state,
					FrameBuffer* destination,
					VGImageFormat format,
					VGint w, VGint h,
					VGbitfield allowedQuality);
		void delete_framebuffer(const GraphicState &state, FrameBuffer* framebuffer);
		void render_to_framebuffer(const GraphicState &state, const FrameBuffer* framebuffer);
		const FrameBuffer* get_internal_framebuffer(gnuVGFrameBuffer selection);
		FrameBuffer *get_temporary_framebuffer(
							   const GraphicState &state,
							   VGImageFormat format,
						       VGint w, VGint h,
						       VGbitfield allowedQuality);
		void return_temporary_framebuffer(FrameBuffer *fbf);

		void copy_framebuffer_to_framebuffer(const GraphicState &state,
							 const FrameBuffer* dst,
						     const FrameBuffer* src,
						     VGint dx, VGint dy,
						     VGint sx, VGint sy,
						     VGint width, VGint height,
						     bool do_blend = false);
		void copy_framebuffer_to_memory(const FrameBuffer* src,
						void *memory, VGint stride,
						VGImageFormat fmt,
						VGint x, VGint y,
						VGint width, VGint height);
		void copy_memory_to_framebuffer(const FrameBuffer* dst,
						const void *memory, VGint stride,
						VGImageFormat fmt,
						VGint x, VGint y,
						VGint width, VGint height);
		void setup_buffers_and_descriptors(	
			eka2l1::drivers::graphics_driver *driver,
			const float *vertex2d, 
			const std::size_t vertex_buffer_size,
			const std::int32_t vertex_stride,
			const float *texcoord2d,
			const std::size_t texcoord_buffer_size,
			const std::int32_t texcoord_stride);

		// used to temporarily switch to another framebuffer
		void save_current_framebuffer();
		void restore_current_framebuffer(const GraphicState &state);

		// getters for stroke data
		VGfloat get_stroke_width();
		VGfloat get_miter_limit();
		VGJoinStyle get_join_style();
		std::vector<VGfloat> get_dash_pattern();
		VGfloat get_dash_phase();
		bool get_dash_phase_reset();

		void vgFlush();
		void vgFinish();

		/* OpenVG equivalent API - Paint Manipulation */
		void vgSetPaint(std::shared_ptr<Paint> p, VGbitfield paintModes);
		std::shared_ptr<Paint> vgGetPaint(VGbitfield paintModes);

		/* OpenVG equivalent API - Matrix Manipulation */
		void vgLoadIdentity(void);
		void vgLoadMatrix(const VGfloat * m);
		void vgGetMatrix(VGfloat * m);
		void vgMultMatrix(const VGfloat * m);
		void vgTranslate(VGfloat tx, VGfloat ty);
		void vgScale(VGfloat sx, VGfloat sy);
		void vgShear(VGfloat shx, VGfloat shy);
		void vgRotate(VGfloat angle);

		/* OpenVG extensions */
		void reset_bounding_box();
		void get_bounding_box(VGfloat *sp_ep);

		// Map the point into the space described by the current matrix
		Point map_point(const Point &p);

		/* OpenVG context setters/getters */
		void vgSetf(VGint paramType, VGfloat value);
		void vgSeti(const GraphicState &state, VGint paramType, VGint value);
		void vgSetfv(VGint paramType, VGint count, const VGfloat *values);
		void vgSetiv(const GraphicState &state, VGint paramType, VGint count, const VGint *values);

		VGfloat vgGetf(VGint paramType);
		VGint vgGeti(VGint paramType);

		VGint vgGetVectorSize(VGint paramType);

		void vgGetfv(VGint paramType, VGint count, VGfloat *values);
		void vgGetiv(VGint paramType, VGint count, VGint *values);

		std::shared_ptr<Object> get_by_handle(VGHandle handle);
		void dereference(const GraphicState &state, VGHandle handle);

		template<typename T>
		std::shared_ptr<T> get(VGHandle handle) {
			auto obj = std::static_pointer_cast<T>(get_by_handle(handle));
			return obj;
		}

		template<class T, class ... Args>
		std::shared_ptr<T> create(Args && ... args) {
			std::shared_ptr<T> ptr = std::make_shared<T>(this, args...);
			VGHandle id = idalloc.get_id();

			objects[ptr->allocate_and_mark_properly_created(id)] = ptr;
			return ptr;
		}

		void on_being_set_current() override;
	};
}

#endif
