#include "os/platform.h"

#ifdef NNGN_PLATFORM_HAS_OPENGL
#include "post.h"

#include "const.h"
#include "graphics/graphics.h"
#include "graphics/shaders.h"
#include "math/vec2.h"

#include "prog.h"
#include "utils.h"
#include "vao.h"

using namespace std::string_view_literals;

namespace nngn {

bool GLPost::init() {
    NNGN_LOG_CONTEXT_CF(GLPost);
    constexpr auto pos_attrs = std::to_array<nngn::VAO::Attrib>({
        {"position", 3}, {{}, 3}, {{}, 3},
    });
    constexpr auto attrs = std::to_array<nngn::VAO::Attrib>({
        {"position", 3}, {{}, 3}, {"tex_coord", 3},
    });
    constexpr auto vbo_data = std::to_array<nngn::Vertex>({
        {{-1, -1, 0}, {}, {0, 0, 0}},
        {{ 3, -1, 0}, {}, {2, 0, 0}},
        {{-1,  3, 0}, {}, {0, 2, 0}},
    });
    constexpr auto ebo_data = std::to_array<u32>({0, 1, 2});
    return LOG_RESULT(glGenTextures, 1, &this->luminance_res_tex.id())
        && LOG_RESULT(glGenTextures, 1, &this->luminance_last_tex.id())
        && this->luminance_res_tex.create(
            GL_TEXTURE_2D, GL_RGB16F, GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE,
            {1, 1, 1}, 0)
        && this->luminance_last_tex.create(
            GL_TEXTURE_2D, GL_RGB16F, GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE,
            {1, 1, 1}, 0)
        && gl_set_obj_name(
            GL_TEXTURE, this->luminance_res_tex.id(), "luminance_res_tex"sv)
        && gl_set_obj_name(
            GL_TEXTURE, this->luminance_last_tex.id(), "luminance_last_tex"sv)
        && this->set_automatic_exposure(false)
        && LOG_RESULT(glGenFramebuffers, 1, &this->luminance_res_fb.id())
        && LOG_RESULT(glBindFramebuffer,
            GL_FRAMEBUFFER, this->luminance_res_fb.id())
        && LOG_RESULT(glFramebufferTexture2D,
            GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
            this->luminance_res_tex.id(), 0)
        && this->vbo.create(
            GL_ARRAY_BUFFER, as_byte_span(&vbo_data), GL_STATIC_DRAW)
        && this->ebo.create(
            GL_ELEMENT_ARRAY_BUFFER, as_byte_span(&ebo_data), GL_STATIC_DRAW)
        && this->luminance_prog.create(
            "src/glsl/gl/post.vert"sv, "src/glsl/gl/luminance.frag"sv,
            nngn::GLSL_GL_POST_VERT, nngn::GLSL_GL_LUMINANCE_FRAG)
        && LOG_RESULT(glUseProgram, this->luminance_prog.id())
        && this->luminance_prog.set_uniform("tex0", 0)
        && this->luminance_prog.set_uniform("tex1", 1)
        && this->luminance_vao.create(this->vbo.id(), this->ebo.id())
        && this->luminance_vao.vertex_attrib_pointers(
            this->luminance_prog, pos_attrs.size(), pos_attrs.data())
        && this->bloom_prog.create(
            "src/glsl/gl/post.vert"sv, "src/glsl/gl/bloom.frag"sv,
            nngn::GLSL_GL_POST_VERT, nngn::GLSL_GL_BLOOM_FRAG)
        && LOG_RESULT(glUseProgram, this->bloom_prog.id())
        && this->bloom_prog.set_uniform("tex", 0)
        && this->bloom_prog.get_uniform_location(
            "threshold", &this->bloom_threshold_loc)
        && this->bloom_filter_vao.create(this->vbo.id(), this->ebo.id())
        && this->bloom_filter_vao.vertex_attrib_pointers(
            this->bloom_prog, attrs.size(), attrs.data())
        && this->blur_prog.create(
            "src/glsl/gl/post.vert"sv, "src/glsl/gl/blur.frag"sv,
            nngn::GLSL_GL_POST_VERT, nngn::GLSL_GL_BLUR_FRAG)
        && LOG_RESULT(glUseProgram, this->blur_prog.id())
        && this->blur_prog.get_uniform_location("dir", &this->blur_dir_loc)
        && this->blur_prog.set_uniform("tex", 0)
        && this->blur_vao.create(this->vbo.id(), this->ebo.id())
        && this->blur_vao.vertex_attrib_pointers(
            this->blur_prog, attrs.size(), attrs.data())
        && this->hdr_prog.create(
            "src/glsl/gl/post.vert"sv, "src/glsl/gl/hdr.frag"sv,
            nngn::GLSL_GL_POST_VERT, nngn::GLSL_GL_HDR_FRAG)
        && LOG_RESULT(glUseProgram, this->hdr_prog.id())
        && this->hdr_prog.set_uniform("color_tex", 0)
        && this->hdr_prog.set_uniform("bloom_tex", 1)
        && this->hdr_prog.set_uniform("luminance_tex", 2)
        && this->hdr_prog.get_uniform_location(
            "exposure_scale", &this->exposure_scale_loc)
        && this->hdr_prog.get_uniform_location(
            "bloom_amount", &this->bloom_amount_loc)
        && this->hdr_prog.get_uniform_location(
            "HDR_mix", &this->HDR_mix_loc)
        && this->hdr_vao.create(this->vbo.id(), this->ebo.id())
        && this->hdr_vao.vertex_attrib_pointers(
            this->hdr_prog, attrs.size(), attrs.data())
        && LOG_RESULT(glBindVertexArray, 0);
}

bool GLPost::destroy() {
    NNGN_LOG_CONTEXT_CF(GLPost);
    return this->color_tex.destroy()
        && this->depth_tex.destroy()
        && this->luminance_tex0.destroy()
        && this->luminance_tex1.destroy()
        && this->blur_tex0.destroy()
        && this->blur_tex1.destroy()
        && this->bloom_blur_tex0.destroy()
        && this->bloom_blur_tex1.destroy()
        && this->color_fb.destroy()
        && this->luminance_fb0.destroy()
        && this->luminance_fb1.destroy()
        && this->blur_fb0.destroy()
        && this->blur_fb1.destroy()
        && this->bloom_blur_fb0.destroy()
        && this->bloom_blur_fb1.destroy()
        && this->bloom_filter_vao.destroy()
        && this->hdr_vao.destroy()
        && this->blur_vao.destroy()
        && this->vbo.destroy()
        && this->ebo.destroy();
}

bool GLPost::update() {
    NNGN_LOG_CONTEXT_CF(GLPost);
    constexpr auto init_tex = [](auto *x, GLenum fmt, ivec2 size_, auto name) {
        return x->destroy()
            && LOG_RESULT(glGenTextures, 1, &x->id())
            && x->create(
                GL_TEXTURE_2D, fmt, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE,
                {size_, 1}, 0)
            && gl_set_obj_name(GL_TEXTURE, x->id(), name);
    };
    constexpr auto init_fb = [](auto *x, auto t) {
        CHECK_RESULT(glGenFramebuffers, 1, &x->id());
        CHECK_RESULT(glBindFramebuffer, GL_FRAMEBUFFER, x->id());
        CHECK_RESULT(glFramebufferTexture2D,
            GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, t, 0);
        return true;
    };
    if(this->flags.check_and_clear(Flag::SIZE_UPDATED)) {
        this->flags.set(Flag::BLOOM_SCALE_UPDATED | Flag::BLUR_SCALE_UPDATED);
        const auto s = static_cast<ivec2>(this->size);
        const auto rhs = static_cast<ivec2>(this->rounded_half_size);
        const bool ok = init_tex(
                &this->color_tex, GL_RGB16F, s, "color_tex"sv)
            && init_tex(
                &this->depth_tex, GL_DEPTH_COMPONENT24, s, "depth_tex"sv)
            && init_tex(
                &this->luminance_tex0, GL_RGB16F, rhs, "luminance_tex0"sv)
            && init_tex(
                &this->luminance_tex1, GL_RGB16F, rhs, "luminance_tex1"sv)
            && init_fb(&this->color_fb, this->color_tex.id())
            && LOG_RESULT(glFramebufferTexture,
                GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, this->depth_tex.id(), 0)
            && init_fb(&this->luminance_fb0, this->luminance_tex0.id())
            && init_fb(&this->luminance_fb1, this->luminance_tex1.id());
        if(!ok)
            return false;
    }
    if(this->flags.check_and_clear(Flag::BLOOM_SCALE_UPDATED)) {
        const auto s = static_cast<ivec2>(
            static_cast<vec2>(this->size) * this->bloom_scale);
        const bool ok = init_tex(
                &this->bloom_blur_tex0, GL_RGB16F, s, "bloom_blur_tex0"sv)
            && init_tex(
                &this->bloom_blur_tex1, GL_RGB16F, s, "bloom_blur_tex1"sv)
            && init_fb(&this->bloom_blur_fb0, this->bloom_blur_tex0.id())
            && init_fb(&this->bloom_blur_fb1, this->bloom_blur_tex1.id());
        if(!ok)
            return false;
    }
    if(this->flags.check_and_clear(Flag::BLUR_SCALE_UPDATED)) {
        const auto s = static_cast<ivec2>(
            static_cast<vec2>(this->size) * this->blur_scale);
        const bool ok = init_tex(&this->blur_tex0, GL_RGB16F, s, "blur_tex0"sv)
            && init_tex(&this->blur_tex1, GL_RGB16F, s, "blur_tex1"sv)
            && init_fb(&this->blur_fb0, this->blur_tex0.id())
            && init_fb(&this->blur_fb1, this->blur_tex1.id());
        if(!ok)
            return false;
    }
    return true;
}

bool GLPost::set_automatic_exposure(bool b) {
    this->flags.set(Flag::AUTOMATIC_EXPOSURE, b);
    if(b)
        return true;
    NNGN_LOG_CONTEXT_CF(GLPost);
    constexpr auto t = GL_TEXTURE_2D;
    constexpr float max = 1.0f / static_cast<float>(NNGN_LUMINANCE_MAX);
    constexpr vec3 v = {max, max, max};
    return LOG_RESULT(glBindTexture, t, this->luminance_last_tex.id())
        && LOG_RESULT(glTexSubImage2D, t, 0, 0, 0, 1, 1, GL_RGB, GL_FLOAT, &v)
        && LOG_RESULT(glBindTexture, t, this->luminance_res_tex.id())
        && LOG_RESULT(glTexSubImage2D, t, 0, 0, 0, 1, 1, GL_RGB, GL_FLOAT, &v);
}

bool GLPost::render() {
    return this->calc_avg_luminance()
        && this->bloom()
        && this->blur()
        && this->HDR();
}

bool GLPost::calc_avg_luminance() {
    if(!(this->enabled() && this->flags.is_set(Flag::AUTOMATIC_EXPOSURE)))
        return true;
    NNGN_LOG_CONTEXT_CF(GLPost);
    nngn::GLDebugGroup group = {"post.luminance"sv};
    CHECK_RESULT(glBindFramebuffer,
        GL_READ_FRAMEBUFFER, this->luminance_res_fb.id());
    CHECK_RESULT(glReadBuffer, GL_COLOR_ATTACHMENT0);
    CHECK_RESULT(glBindTexture, GL_TEXTURE_2D, this->luminance_last_tex.id());
    CHECK_RESULT(glCopyTexSubImage2D, GL_TEXTURE_2D, 0, 0, 0, 0, 0, 1, 1);
    auto width = static_cast<GLint>(this->size.x);
    auto height = static_cast<GLint>(this->size.y);
    auto new_width = static_cast<GLint>(this->rounded_half_size.x);
    auto new_height = static_cast<GLint>(this->rounded_half_size.y);
    GLuint src = this->color_fb.id(), dst = this->luminance_fb0.id();
    const auto blit = [&width, &height, &new_width, &new_height, &src, &dst] {
        return LOG_RESULT(glBindFramebuffer, GL_READ_FRAMEBUFFER, src)
            && LOG_RESULT(glBindFramebuffer, GL_DRAW_FRAMEBUFFER, dst)
            && LOG_RESULT(glBlitFramebuffer,
                0, 0, width, height,
                0, 0, new_width, new_height,
                GL_COLOR_BUFFER_BIT, GL_LINEAR);
    };
    if(!blit())
        return false;
    src = this->luminance_fb1.id();
    auto src_tex = this->luminance_tex1.id();
    auto dst_tex = this->luminance_tex0.id();
    while(width > 2 || height > 2) {
        std::swap(src, dst);
        std::swap(src_tex, dst_tex);
        width = std::exchange(new_width, std::max(new_width >> 1, 1));
        height = std::exchange(new_height, std::max(new_height >> 1, 1));
        if(!blit())
            return false;
    }
    CHECK_RESULT(glBindFramebuffer,
        GL_FRAMEBUFFER, this->luminance_res_fb.id());
    CHECK_RESULT(glActiveTexture, GL_TEXTURE0);
    CHECK_RESULT(glBindTexture, GL_TEXTURE_2D, this->luminance_last_tex.id());
    CHECK_RESULT(glActiveTexture, GL_TEXTURE1);
    CHECK_RESULT(glBindTexture, GL_TEXTURE_2D, dst_tex);
    CHECK_RESULT(glViewport, 0, 0, 1, 1);
    CHECK_RESULT(glUseProgram, this->luminance_prog.id());
    CHECK_RESULT(glBindVertexArray, this->luminance_vao.id());
    CHECK_RESULT(glDrawElements, GL_TRIANGLES, 3, GL_UNSIGNED_INT, nullptr);
    return true;
}

bool GLPost::bloom() const {
    if(!this->bloom_enabled())
        return true;
    NNGN_LOG_CONTEXT_CF(GLPost);
    nngn::GLDebugGroup group = {"post.bloom"sv};
    const auto width = static_cast<GLsizei>(
        static_cast<float>(this->size.x) * this->bloom_scale);
    const auto height = static_cast<GLsizei>(
        static_cast<float>(this->size.y) * this->bloom_scale);
    CHECK_RESULT(glBindFramebuffer, GL_FRAMEBUFFER, this->bloom_blur_fb0.id());
    CHECK_RESULT(glViewport, 0, 0, width, height);
    CHECK_RESULT(glActiveTexture, GL_TEXTURE1);
    CHECK_RESULT(glBindTexture, GL_TEXTURE_2D, 0);
    CHECK_RESULT(glActiveTexture, GL_TEXTURE0);
    CHECK_RESULT(glBindTexture, GL_TEXTURE_2D, this->color_tex.id());
    CHECK_RESULT(glUseProgram, this->bloom_prog.id());
    CHECK_RESULT(glUniform1f, this->bloom_threshold_loc, this->bloom_threshold);
    CHECK_RESULT(glBindVertexArray, this->bloom_filter_vao.id());
    CHECK_RESULT(glDrawElements, GL_TRIANGLES, 3, GL_UNSIGNED_INT, nullptr);
    CHECK_RESULT(glUseProgram, this->blur_prog.id());
    CHECK_RESULT(glBindVertexArray, this->blur_vao.id());
    for(std::size_t i = 0, n = this->bloom_blur_passes; i < n; ++i) {
        CHECK_RESULT(glBindTexture, GL_TEXTURE_2D, this->bloom_blur_tex0.id());
        CHECK_RESULT(glBindFramebuffer,
            GL_FRAMEBUFFER, this->bloom_blur_fb1.id());
        CHECK_RESULT(glUniform2f, this->blur_dir_loc, this->bloom_blur_size, 0);
        CHECK_RESULT(glDrawElements, GL_TRIANGLES, 3, GL_UNSIGNED_INT, nullptr);
        CHECK_RESULT(glBindTexture, GL_TEXTURE_2D, this->bloom_blur_tex1.id());
        CHECK_RESULT(glBindFramebuffer,
            GL_FRAMEBUFFER, this->bloom_blur_fb0.id());
        CHECK_RESULT(glUniform2f, this->blur_dir_loc, 0, this->bloom_blur_size);
        CHECK_RESULT(glDrawElements, GL_TRIANGLES, 3, GL_UNSIGNED_INT, nullptr);
    }
    return true;
}

bool GLPost::blur() const {
    if(!this->blur_enabled())
        return true;
    NNGN_LOG_CONTEXT_CF(GLPost);
    nngn::GLDebugGroup group = {"post.blur"sv};
    CHECK_RESULT(glActiveTexture, GL_TEXTURE0);
    CHECK_RESULT(glBindTexture, GL_TEXTURE_2D, this->color_tex.id());
    CHECK_RESULT(glBindFramebuffer, GL_READ_FRAMEBUFFER, this->color_fb.id());
    CHECK_RESULT(glBindFramebuffer, GL_DRAW_FRAMEBUFFER, this->blur_fb0.id());
    const auto fb_width = static_cast<GLsizei>(this->size.x);
    const auto fb_height = static_cast<GLsizei>(this->size.y);
    const auto width = static_cast<GLsizei>(
        static_cast<float>(fb_width) * this->blur_scale);
    const auto height = static_cast<GLsizei>(
        static_cast<float>(fb_height) * this->blur_scale);
    CHECK_RESULT(glBlitFramebuffer,
        0, 0, fb_width, fb_height,
        0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    CHECK_RESULT(glViewport, 0, 0, width, height);
    CHECK_RESULT(glUseProgram, this->blur_prog.id());
    CHECK_RESULT(glBindVertexArray, this->blur_vao.id());
    for(std::size_t i = 0, n = this->blur_passes; i < n; ++i) {
        CHECK_RESULT(glBindTexture, GL_TEXTURE_2D, this->blur_tex0.id());
        CHECK_RESULT(glBindFramebuffer, GL_FRAMEBUFFER, this->blur_fb1.id());
        CHECK_RESULT(glUniform2f, this->blur_dir_loc, this->blur_size, 0);
        CHECK_RESULT(glDrawElements, GL_TRIANGLES, 3, GL_UNSIGNED_INT, nullptr);
        CHECK_RESULT(glBindTexture, GL_TEXTURE_2D, this->blur_tex1.id());
        CHECK_RESULT(glBindFramebuffer, GL_FRAMEBUFFER, this->blur_fb0.id());
        CHECK_RESULT(glUniform2f, this->blur_dir_loc, 0, this->blur_size);
        CHECK_RESULT(glDrawElements, GL_TRIANGLES, 3, GL_UNSIGNED_INT, nullptr);
    }
    CHECK_RESULT(glBindFramebuffer, GL_READ_FRAMEBUFFER, this->blur_fb0.id());
    CHECK_RESULT(glBindFramebuffer, GL_DRAW_FRAMEBUFFER, this->color_fb.id());
    CHECK_RESULT(glBlitFramebuffer,
        0, 0, width, height, 0, 0, fb_width, fb_height,
        GL_COLOR_BUFFER_BIT, GL_LINEAR);
    return true;
}

bool GLPost::HDR() const {
    if(!this->enabled())
        return true;
    NNGN_LOG_CONTEXT_CF(GLPost);
    nngn::GLDebugGroup group = {"post.HDR"sv};
    CHECK_RESULT(glBindFramebuffer, GL_FRAMEBUFFER, 0);
    CHECK_RESULT(glViewport,
        0, 0,
        static_cast<GLsizei>(this->size.x),
        static_cast<GLsizei>(this->size.y));
    CHECK_RESULT(glClear, GL_COLOR_BUFFER_BIT);
    CHECK_RESULT(glActiveTexture, GL_TEXTURE0);
    CHECK_RESULT(glBindTexture, GL_TEXTURE_2D, this->color_tex.id());
    CHECK_RESULT(glActiveTexture, GL_TEXTURE1);
    CHECK_RESULT(glBindTexture, GL_TEXTURE_2D, this->bloom_blur_tex0.id());
    CHECK_RESULT(glActiveTexture, GL_TEXTURE2);
    CHECK_RESULT(glBindTexture, GL_TEXTURE_2D, this->luminance_res_tex.id());
    CHECK_RESULT(glUseProgram, this->hdr_prog.id());
    CHECK_RESULT(glUniform1f, this->exposure_scale_loc, this->exposure);
    CHECK_RESULT(glUniform1f, this->bloom_amount_loc, this->bloom_amount);
    CHECK_RESULT(glUniform1f, this->HDR_mix_loc, this->HDR_mix);
    CHECK_RESULT(glBindVertexArray, this->hdr_vao.id());
    CHECK_RESULT(glDrawElements, GL_TRIANGLES, 3, GL_UNSIGNED_INT, nullptr);
    return true;
}

}
#endif
