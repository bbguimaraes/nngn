#ifndef NNGN_RENDER_RENDER_H
#define NNGN_RENDER_RENDER_H

#include <bit>
#include <vector>

#include <sol/forward.hpp>

#include "graphics/graphics.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include "utils/flags.h"

#include "renderers.h"

struct Entity;

namespace nngn {

class Textures;

class Renderers {
    enum Flag : u8 {
        SPRITES_UPDATED = 1u << 0,
        RECT_UPDATED = 1u << 1,
    };
    Textures *textures = nullptr;
    Graphics *graphics = nullptr;
    Flags<Flag> flags = {};
    std::vector<SpriteRenderer> sprites = {};
    u32
        sprite_vbo = {}, sprite_ebo = {},
        box_vbo = {}, box_ebo = {};
public:
    enum Debug : u8 {
        RECT = 1u << 0, N_DEBUG = 1,
    };
private:
    Flags<Debug> m_debug = {};
public:
    static void gen_quad_idxs(u64 i, u64 n, u32 *p);
    static void gen_quad_verts(
        Vertex **p, vec2 bl, vec2 tr, float z, u32 tex, vec2 uv0, vec2 uv1);
    static void gen_quad_verts(
        Vertex **p, vec2 bl, vec2 tr, float z, vec3 color);
    void init(Textures *t);
    auto max_sprites() const { return this->sprites.capacity(); }
    auto debug() const { return this->m_debug.t; }
    std::size_t n() const;
    std::size_t n_sprites() const { return this->sprites.size(); }
    bool set_max_sprites(std::size_t n);
    void set_debug(std::underlying_type_t<Debug> d);
    bool set_graphics(Graphics *g);
    Renderer *load(const sol::stack_table &t);
    void remove(Renderer *p);
    bool update();
};

inline void Renderers::gen_quad_idxs(u64 i_64, u64 n, u32 *p) {
    constexpr std::size_t n_verts = 4, log2 = std::countr_zero(n_verts);
    auto i = static_cast<u32>(i_64) << log2;
    for(const auto e = i + (n << log2); i != e; i += n_verts) {
        *p++ = i    ; *(p++) = i + 1; *(p++) = i + 2;
        *p++ = i + 2; *(p++) = i + 1; *(p++) = i + 3;
    }
}

inline void Renderers::gen_quad_verts(
    Vertex **pp, vec2 bl, vec2 tr, float z, vec3 color
) {
    auto *p = *pp;
    *p++ = {{bl,         z}, color};
    *p++ = {{tr.x, bl.y, z}, color};
    *p++ = {{bl.x, tr.y, z}, color};
    *p++ = {{tr,         z}, color};
    *pp = p;
}

inline void Renderers::gen_quad_verts(
    Vertex **pp, vec2 bl, vec2 tr, float z,
    u32 tex, vec2 uv0, vec2 uv1
) {
    const auto tex_f = static_cast<float>(tex);
    auto *p = *pp;
    *p++ = {{bl,         z}, {uv0.x, uv0.y, tex_f}};
    *p++ = {{tr.x, bl.y, z}, {uv1.x, uv0.y, tex_f}};
    *p++ = {{bl.x, tr.y, z}, {uv0.x, uv1.y, tex_f}};
    *p++ = {{tr,         z}, {uv1.x, uv1.y, tex_f}};
    *pp = p;
}

}

#endif
