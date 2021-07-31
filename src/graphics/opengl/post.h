#ifndef NNGN_GRAPHICS_OPENGL_POST_H
#define NNGN_GRAPHICS_OPENGL_POST_H

#include "graphics/graphics.h"
#include "utils/flags.h"
#include "utils/log.h"

#include "prog.h"
#include "resource.h"
#include "utils.h"
#include "vao.h"

namespace nngn {

/** Post-processing operations. */
class GLPost {
public:
    /** Single-step, swap-chain-independent initialization. */
    bool init();
    /** Releases all resources. */
    bool destroy();
    /** Signals that the frame buffer has been resized. */
    void resize(uvec2 size);
    bool set_automatic_exposure(bool b);
    void set_exposure(float e) { this->exposure = e; }
    void set_bloom_downscale(std::size_t d);
    void set_bloom_threshold(float t) { this->bloom_threshold = t; }
    void set_bloom_blur_size(float n) { this->bloom_blur_size = n; }
    void set_bloom_blur_passes(std::size_t n) { this->bloom_blur_passes = n; }
    void set_bloom_amount(float a) { this->bloom_amount = a; }
    void set_blur_downscale(std::size_t d);
    void set_blur_size(float n) { this->blur_size = n; }
    void set_blur_passes(std::size_t n) { this->blur_passes = n; }
    void set_HDR_mix(float m) { this->HDR_mix = m; }
    /** Updates resources as necessary. */
    bool update();
    /**
     * Binds the appropriate frame buffer according to the configuration.
     * May bind either the internal color/depth textures or the default frame
     * buffer.
     */
    bool bind_frame_buffer();
    /** Renders all enabled steps. */
    bool render();
private:
    enum Flag : u8 {
        AUTOMATIC_EXPOSURE  = 1u << 0,
        SIZE_UPDATED        = 1u << 1,
        BLOOM_SCALE_UPDATED = 1u << 2,
        BLUR_SCALE_UPDATED  = 1u << 3,
    };
    bool enabled() const;
    bool HDR_enabled() const;
    bool bloom_enabled() const;
    bool blur_enabled() const;
    bool resize_frame_buffers();
    bool calc_avg_luminance();
    bool bloom() const;
    bool blur() const;
    bool HDR() const;
    Flags<Flag> flags = {};
    float exposure = Graphics::DEFAULT_EXPOSURE;
    float bloom_scale =
        1.0f / static_cast<float>(Graphics::DEFAULT_BLOOM_DOWNSCALE);
    float bloom_threshold = Graphics::DEFAULT_BLOOM_THRESHOLD;
    float bloom_blur_size = Graphics::DEFAULT_BLOOM_BLUR_SIZE;
    std::size_t bloom_blur_passes = Graphics::DEFAULT_BLOOM_BLUR_PASSES;
    float bloom_amount = 0;
    float blur_scale =
        1.0f / static_cast<float>(Graphics::DEFAULT_BLUR_DOWNSCALE);
    float blur_size = 0;
    std::size_t blur_passes = 0;
    float HDR_mix = 0;
    GLProgram luminance_prog = {};
    GLProgram bloom_prog = {};
    GLProgram blur_prog = {};
    GLProgram hdr_prog = {};
    GLTexArray
        color_tex = {}, depth_tex = {},
        luminance_tex0 = {}, luminance_tex1 = {},
        luminance_last_tex = {}, luminance_res_tex = {},
        blur_tex0 = {}, blur_tex1 = {},
        bloom_blur_tex0 = {}, bloom_blur_tex1 = {};
    GLFrameBuffer
        color_fb = {},
        luminance_fb0 = {}, luminance_fb1 = {}, luminance_res_fb = {},
        blur_fb0 = {}, blur_fb1 = {},
        bloom_blur_fb0 = {}, bloom_blur_fb1 = {};
    int bloom_threshold_loc = -1, bloom_amount_loc = -1, blur_dir_loc = -1;
    int exposure_scale_loc = -1, HDR_mix_loc = -1;
    VAO luminance_vao = {}, bloom_filter_vao = {}, blur_vao = {}, hdr_vao = {};
    GLBuffer vbo = {}, ebo = {};
    uvec2 size = {}, half_size = {}, rounded_half_size = {};
};

inline void GLPost::resize(uvec2 s) {
    this->size = s;
    this->half_size = s / 2u;
    this->rounded_half_size = {
        std::bit_floor(s.x) >> 1,
        std::bit_floor(s.y) >> 1,
    };
    this->flags.set(Flag::SIZE_UPDATED);
}

inline bool GLPost::enabled() const {
    return this->HDR_enabled() || this->bloom_enabled() || this->blur_enabled();
}

inline bool GLPost::HDR_enabled() const {
    return this->HDR_mix != 0;
}

inline bool GLPost::bloom_enabled() const {
    return this->bloom_amount != 0;
}

inline bool GLPost::blur_enabled() const {
    return this->blur_passes && this->blur_size != 0;
}

inline void GLPost::set_bloom_downscale(std::size_t d) {
    this->bloom_scale = 1.0f / static_cast<float>(d);
    this->flags.set(Flag::BLOOM_SCALE_UPDATED);
}

inline void GLPost::set_blur_downscale(std::size_t d) {
    this->blur_scale = 1.0f / static_cast<float>(d);
    this->flags.set(Flag::BLUR_SCALE_UPDATED);
}

inline bool GLPost::bind_frame_buffer() {
    NNGN_LOG_CONTEXT_CF(GLPost);
    return LOG_RESULT(glBindFramebuffer,
        GL_FRAMEBUFFER, this->enabled() ? this->color_fb.id() : 0);
}

}

#endif
