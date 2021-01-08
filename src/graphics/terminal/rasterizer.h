#ifndef NNGN_GRAPHICS_TERMINAL_RASTERIZER_H
#define NNGN_GRAPHICS_TERMINAL_RASTERIZER_H

#include "graphics/graphics.h"
#include "math/mat4.h"
#include "math/vec2.h"

#include "texture.h"

namespace nngn::term {

class FrameBuffer;

/** Axis-aligned sprite rasterizer with texture sampling. */
class Rasterizer {
public:
    using Mode = nngn::Graphics::TerminalMode;
    explicit Rasterizer(Mode m) : mode{m} {}
    /** World projection matrix. */
    mat4 proj(void) const { return this->m_proj; }
    /** UI projection matrix. */
    mat4 hud_proj(void) const { return this->m_hud_proj; }
    /** Updates internal camera matrices. */
    void update_camera(
        uvec2 term_size, uvec2 window_size,
        mat4 proj, mat4 hud_proj, mat4 view);
    /** Rasterizes a VBO/EBO pair containing textured quad. data. */
    void sprite(
        std::span<const Vertex> vbo, std::span<const u32> ebo, mat4 proj,
        std::span<const Texture> textures, FrameBuffer *fb) const;
    /** Rasterizes a VBO/EBO pair containing text data. */
    void font(
        std::span<const Vertex> vbo, std::span<const u32> ebo, mat4 proj,
        std::span<const Texture> font, FrameBuffer *fb) const;
private:
    mat4 m_proj = {}, m_hud_proj = {};
    Mode mode;
};

}

#endif
