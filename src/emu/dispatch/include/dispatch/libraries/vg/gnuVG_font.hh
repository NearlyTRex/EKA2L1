/*
 * gnuVG - a free Vector Graphics library
 * Copyright (C) 2016 by Anton Persson
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

#ifndef __GNUVG_FONT_HH
#define __GNUVG_FONT_HH

#include <dispatch/libraries/vg/gnuVG_context.hh>
#include <dispatch/libraries/vg/gnuVG_object.hh>
#include <dispatch/libraries/vg/gnuVG_image.hh>
#include <dispatch/libraries/vg/gnuVG_path.hh>
#include <dispatch/libraries/vg/gnuVG_paint.hh>

#include <SkylineBinPack.h>

#include <vector>
#include <string>

namespace gnuVG {

	class FontCache : public rbp::SkylineBinPack {
	public:
		Context::FrameBuffer framebuffer;
		Context *context;

		struct Rect {
			int x, y, width, height;
			int offset_x, offset_y;
			bool rotated;
		};

	private:
		std::map<VGuint, Rect> cache;
		float tex_mtrx[9];

	public:
		FontCache(Context *context, const GraphicState &state, int w, int h);
		virtual ~FontCache() {}

		const float* get_texture_matrix() const {
			return tex_mtrx;
		}

		Rect pack(VGuint charcode, int offset_x, int offset_y, int w, int h) {
			auto r = Insert(w, h, LevelMinWasteFit);

			Rect rr;
			rr.x = r.x;
			rr.y = r.y;
			rr.offset_x = offset_x;
			rr.offset_y = offset_y;
			rr.width = r.width;
			rr.height = r.height;

			rr.rotated = (w != h && rr.width != w) ? true : false;

			if(rr.width != 0 && rr.height != 0)
				cache[charcode] = rr;

			return rr;
		}

		bool lookup(VGuint charcode, Rect &result) {
			auto found = cache.find(charcode);
			if(found == cache.end())
				return false;

			result = (*found).second;
			return true;
		}
	};

	class Font : public Object {
	private:
		class Glyph {
		public:
			bool isHinted = false;
			std::shared_ptr<Path> path;
			std::shared_ptr<Image> image;
			VGfloat origin[2], escapement[2];

			Glyph();
			~Glyph();

			void clear();
			void set_path(std::shared_ptr<Path> _path,
				      VGboolean _isHinted,
				      const VGfloat *_origin, const VGfloat *_escapement);
			void set_image(std::shared_ptr<Image> _image,
				       const VGfloat *_origin, const VGfloat *_escapement);
		};

		std::shared_ptr<Paint> cache_render_paint;

		std::vector<Glyph*> glyphs;
		std::vector<VGfloat> adjustments_x;
		std::vector<VGuint> glyph_indices;
		std::map<int, FontCache *> font_caches;
		void secure_max_glyphIndex(VGuint glyphIndex);

		int get_fc_scale(); // font cache scalex
		FontCache* get_font_cache(const GraphicState &state, int fc_scale);

		void prefill_cache(int fc_scale, const std::vector<VGuint> &glyph_indices);

	public:
		Font(Context *context, VGint glyphCapacityHint);
		virtual ~Font();

		/* OpenVG equivalent API */
		virtual void vgSetParameterf(VGint paramType, VGfloat value);
		virtual void vgSetParameteri(VGint paramType, VGint value);
		virtual void vgSetParameterfv(VGint paramType, VGint count, const VGfloat *values);
		virtual void vgSetParameteriv(VGint paramType, VGint count, const VGint *values);

		virtual VGfloat vgGetParameterf(VGint paramType);
		virtual VGint vgGetParameteri(VGint paramType);

		virtual VGint vgGetParameterVectorSize(VGint paramType);

		virtual void vgGetParameterfv(VGint paramType, VGint count, VGfloat *values);
		virtual void vgGetParameteriv(VGint paramType, VGint count, VGint *values);

		void vgSetGlyphToPath(VGuint glyphIndex,
				      std::shared_ptr<Path> path,
				      VGboolean isHinted,
				      const VGfloat *glyphOrigin,
				      const VGfloat *escapement);
		void vgSetGlyphToImage(VGuint glyphIndex,
				       std::shared_ptr<Image> image,
				       const VGfloat *glyphOrigin,
				       const VGfloat *escapement);
		void vgClearGlyph(VGuint glyphIndex);
		void vgDrawGlyph(const GraphicState &state, VGuint glyphIndex,
				 VGbitfield paintModes,
				 VGboolean allowAutoHinting);
		void vgDrawGlyphs(const GraphicState &state, VGint glyphCount,
				  const VGuint *glyphIndices,
				  const VGfloat *adjustments_x,
				  const VGfloat *adjustments_y,
				  VGbitfield paintModes,
				  VGboolean allowAutoHinting);

	};
}

#endif
