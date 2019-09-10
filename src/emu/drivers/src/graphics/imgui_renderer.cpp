/*
 * Copyright (c) 2019 EKA2L1 Team.
 * 
 * This file is part of EKA2L1 project
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <drivers/graphics/graphics.h>
#include <drivers/graphics/imgui_renderer.h>

#include <imgui.h>
#include <imgui_internal.h>

namespace eka2l1::drivers {
    static const char *gl_vertex_shader_renderer = "#version 330\n"
                                                   "uniform mat4 ProjMtx;\n"
                                                   "in vec2 Position;\n"
                                                   "in vec2 UV;\n"
                                                   "in vec4 Color;\n"
                                                   "out vec2 Frag_UV;\n"
                                                   "out vec4 Frag_Color;\n"
                                                   "void main()\n"
                                                   "{\n"
                                                   "	Frag_UV = UV;\n"
                                                   "	Frag_Color = Color;\n"
                                                   "	gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
                                                   "}\n";

    static const char *gl_fragment_shader_renderer = "#version 330\n"
                                                     "uniform sampler2D Texture;\n"
                                                     "in vec2 Frag_UV;\n"
                                                     "in vec4 Frag_Color;\n"
                                                     "out vec4 Out_Color;\n"
                                                     "void main()\n"
                                                     "{\n"
                                                     "	Out_Color = Frag_Color * texture( Texture, Frag_UV.st);\n"
                                                     "}\n";

    static void get_render_shader(graphics_driver *driver, const char *&vert_dat, const char *&frag_dat) {
        switch (driver->get_current_api()) {
        case graphic_api::opengl: {
            vert_dat = gl_vertex_shader_renderer;
            frag_dat = gl_fragment_shader_renderer;

            break;
        }

        default:
            break;
        }
    }

    void imgui_renderer::init(graphics_driver *driver, graphics_command_list_builder *builder) {
        const char *vert_dat = nullptr;
        const char *frag_dat = nullptr;

        proj_matrix_loc = -1;

        get_render_shader(driver, vert_dat, frag_dat);
        shader = create_program(driver, vert_dat, 0, frag_dat, 0, &smeta);

        auto &io = ImGui::GetIO();
        unsigned char *pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

        drivers::handle font_tex = create_texture(driver, 2, 0, drivers::texture_format::rgba, drivers::texture_format::rgba,
            drivers::texture_data_type::ubyte, pixels, eka2l1::vec3(width, height, 0));

        io.Fonts->TexID = reinterpret_cast<ImTextureID>(font_tex);

        // Create buffers. We will upload them later...
        vbo = create_buffer(driver, 120, drivers::buffer_hint::vertex_buffer, static_cast<drivers::buffer_upload_hint>(buffer_upload_stream | buffer_upload_draw));
        ibo = create_buffer(driver, 120, drivers::buffer_hint::index_buffer, static_cast<drivers::buffer_upload_hint>(buffer_upload_stream | buffer_upload_draw));

        // Attach descriptor
        attribute_descriptor descriptor[3];
        descriptor[0].location = 0;
        descriptor[0].set_format(2, data_format::sfloat);
        descriptor[0].offset = offsetof(ImDrawVert, pos);

        descriptor[1].location = 1;
        descriptor[1].set_format(2, data_format::sfloat);
        descriptor[1].offset = offsetof(ImDrawVert, uv);

        descriptor[2].location = 2;
        descriptor[2].set_format(4, data_format::byte);
        descriptor[2].offset = offsetof(ImDrawVert, col);

        builder->attach_descriptors(vbo, 20, false, descriptor, 3);

        // If meta is not available, there is a high chance that this is in a uniform buffer.
        if (smeta.is_available()) {
            proj_matrix_loc = smeta.get_uniform_binding("ProjMtx");
        }
    }

    void imgui_renderer::render(graphics_driver *driver, graphics_command_list_builder *cmd_builder, ImDrawData *draw_data) {
        auto &io = ImGui::GetIO();

        // Scale clip rects
        auto fb_width = static_cast<int>(io.DisplaySize.x * io.DisplayFramebufferScale.x);
        auto fb_height = static_cast<int>(io.DisplaySize.y * io.DisplayFramebufferScale.y);
        draw_data->ScaleClipRects(io.DisplayFramebufferScale);

        // Backup GL state
        cmd_builder->backup_state();
        cmd_builder->clear({ 0, 0, 0, 0 }, clear_bit_color_buffer);

        // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled

        // glEnable(GL_BLEND);
        cmd_builder->set_blend_mode(true);

        //glBlendEquation(GL_FUNC_ADD);
        //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        cmd_builder->blend_formula(blend_equation::add, blend_equation::add, blend_factor::frag_out_alpha,
            blend_factor::one_minus_frag_out_alpha, blend_factor::frag_out_alpha, blend_factor::one_minus_frag_out_alpha);

        //glDisable(GL_CULL_FACE);
        cmd_builder->set_cull_mode(false);

        // glDisable(GL_DEPTH_TEST);
        cmd_builder->set_depth(false);

        //glEnable(GL_SCISSOR_TEST);
        cmd_builder->set_invalidate(true);

        //glActiveTexture(GL_TEXTURE0);

        // Setup viewport, orthographic projection matrix
        //glViewport(0, 0, static_cast<GLsizei>(fb_width), static_cast<GLsizei>(fb_height));
        cmd_builder->set_viewport(eka2l1::rect{ eka2l1::vec2{ 0, 0 }, eka2l1::vec2{ fb_width, fb_height } });

        const float ortho_projection[4][4] = {
            { 2.0f / io.DisplaySize.x, 0.0f, 0.0f, 0.0f },
            { 0.0f, 2.0f / -io.DisplaySize.y, 0.0f, 0.0f },
            { 0.0f, 0.0f, -1.0f, 0.0f },
            { -1.0f, 1.0f, 0.0f, 1.0f },
        };

        //shader.use();
        cmd_builder->use_program(shader);

        if (proj_matrix_loc != -1) {
            // glUniformMatrix4fv(attrib_loc_proj_matrix, 1, GL_FALSE, &ortho_projection[0][0]);
            cmd_builder->set_uniform(shader, proj_matrix_loc, drivers::shader_set_var_type::mat4, &ortho_projection[0][0], 64);
        }

        //glBindVertexArray(vao_handle);

        // Upload data as contiguous chunk
        std::vector<const void *> vbo_pointers;
        std::vector<std::uint16_t> vbo_sizes;

        std::vector<const void *> ibo_pointers;
        std::vector<std::uint16_t> ibo_sizes;

        vbo_pointers.resize(draw_data->CmdListsCount);
        vbo_sizes.resize(draw_data->CmdListsCount);
        ibo_pointers.resize(draw_data->CmdListsCount);
        ibo_sizes.resize(draw_data->CmdListsCount);

        for (auto n = 0; n < draw_data->CmdListsCount; n++) {
            const ImDrawList *cmd_list = draw_data->CmdLists[n];
            vbo_pointers[n] = &cmd_list->VtxBuffer.front();
            vbo_sizes[n] = static_cast<std::uint16_t>(cmd_list->VtxBuffer.size());

            ibo_pointers[n] = &cmd_list->IdxBuffer.front();
            ibo_sizes[n] = static_cast<std::uint16_t>(cmd_list->IdxBuffer.size());
        }

        cmd_builder->update_buffer_data(vbo, 0, draw_data->CmdListsCount, vbo_pointers.data(), vbo_sizes.data());
        cmd_builder->update_buffer_data(ibo, 0, draw_data->CmdListsCount, ibo_pointers.data(), ibo_sizes.data());

        // Bind them for draw
        cmd_builder->bind_buffer(vbo);
        cmd_builder->bind_buffer(ibo);

        int elem_offset = 0;
        int vert_offset = 0;

        for (auto n = 0; n < draw_data->CmdListsCount; n++) {
            const ImDrawList *cmd_list = draw_data->CmdLists[n];

            for (auto pcmd = cmd_list->CmdBuffer.begin(); pcmd != cmd_list->CmdBuffer.end(); pcmd++) {
                if (pcmd->UserCallback) {
                    pcmd->UserCallback(cmd_list, pcmd);
                } else {
                    // glBindTexture(GL_TEXTURE_2D, static_cast<const GLuint>(reinterpret_cast<std::ptrdiff_t>(pcmd->TextureId)));
                    cmd_builder->bind_texture(reinterpret_cast<drivers::handle>(pcmd->TextureId), 0);

                    //glScissor(static_cast<int>(pcmd->ClipRect.x), static_cast<int>(fb_height - pcmd->ClipRect.w),
                    //    static_cast<int>(pcmd->ClipRect.z - pcmd->ClipRect.x),
                    //   static_cast<int>(pcmd->ClipRect.w - pcmd->ClipRect.y));

                    eka2l1::rect inv_rect(
                        eka2l1::vec2{ static_cast<int>(pcmd->ClipRect.x), static_cast<int>(pcmd->ClipRect.y) },
                        eka2l1::vec2{ static_cast<int>(pcmd->ClipRect.z - pcmd->ClipRect.x), static_cast<int>(pcmd->ClipRect.w - pcmd->ClipRect.y) });
                    
                    cmd_builder->invalidate_rect(inv_rect);

                    //glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buf_offset);
                    cmd_builder->draw_indexed(drivers::graphics_primitive_mode::triangles, pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? drivers::data_format::word : drivers::data_format::uint,
                        elem_offset, vert_offset);

                    // Support for OpenGL < 4.3
                    // GLenum err = 0;

                    // err = glGetError();
                    // while (err != GL_NO_ERROR) {
                    //    LOG_ERROR("Error in OpenGL operation: {}", gl_error_to_string(err));
                    //    err = glGetError();
                    //}
                }

                elem_offset += pcmd->ElemCount;
            }

            vert_offset += cmd_list->VtxBuffer.Size;
        }

        // Restore modified state
        cmd_builder->load_backup_state();
    }

    void imgui_renderer::deinit(graphics_command_list_builder *builder) {
        builder->destroy(this->shader);
        builder->destroy(vbo);
        builder->destroy(ibo);
    }
}
