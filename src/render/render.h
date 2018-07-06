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

class Renderers {
    enum Flag : u8 {
        SPRITES_UPDATED = 1u << 0,
    };
    Graphics *graphics = nullptr;
    Flags<Flag> flags = {};
    std::vector<SpriteRenderer> sprites = {};
    u32 sprite_vbo = {}, sprite_ebo = {};
public:
    static void gen_quad_idxs(u64 i, u64 n, u32 *p);
    static void gen_quad_verts(Vertex **p, vec2 bl, vec2 tr, float z);
    auto max_sprites() const { return this->sprites.capacity(); }
    std::size_t n() const;
    std::size_t n_sprites() const { return this->sprites.size(); }
    bool set_max_sprites(std::size_t n);
    bool set_graphics(Graphics *g);
    Renderer *load(const sol::stack_table &t);
    void remove(Renderer *p);
    bool update();
};

inline void Renderers::gen_quad_idxs(u64 i_64, u64 n, u32 *p) {
    constexpr std::size_t n_verts = 4, log2 = std::countr_zero(n_verts);
    auto i = static_cast<std::uint32_t>(i_64) << log2;
    for(const auto e = i + (n << log2); i != e; i += n_verts) {
        *p++ = i    ; *(p++) = i + 1; *(p++) = i + 2;
        *p++ = i + 2; *(p++) = i + 1; *(p++) = i + 3;
    }
}

inline void Renderers::gen_quad_verts(Vertex **pp, vec2 bl, vec2 tr, float z) {
    auto *p = *pp;
    *p++ = {{bl,         z}, {1, 0, 0}};
    *p++ = {{tr.x, bl.y, z}, {0, 1, 0}};
    *p++ = {{bl.x, tr.y, z}, {0, 0, 1}};
    *p++ = {{tr,         z}, {1, 1, 1}};
    *pp = p;
}

}

#endif
