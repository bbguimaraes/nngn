#ifndef NNGN_RENDER_GEN_H
#define NNGN_RENDER_GEN_H

#include "graphics/graphics.h"

#include "renderers.h"

namespace nngn {

struct Gen {
    // Primitives
    static void quad_indices(u64 i, u64 n, u32 *p);
    static void quad_vertices(Vertex **p, vec2 bl, vec2 tr, float z);
    // Renderers
    static void sprite(Vertex **p, SpriteRenderer *x);
};

inline void Gen::quad_indices(u64 i_64, u64 n, u32 *p) {
    constexpr std::size_t n_verts = 4, log2 = std::countr_zero(n_verts);
    auto i = static_cast<u32>(i_64) << log2;
    for(const auto e = i + (n << log2); i != e; i += n_verts) {
        *p++ = i    ; *(p++) = i + 1; *(p++) = i + 2;
        *p++ = i + 2; *(p++) = i + 1; *(p++) = i + 3;
    }
}

inline void Gen::quad_vertices(Vertex **pp, vec2 bl, vec2 tr, float z) {
    auto *p = *pp;
    *p++ = {{bl,         z}, {1, 0, 0}};
    *p++ = {{tr.x, bl.y, z}, {0, 1, 0}};
    *p++ = {{bl.x, tr.y, z}, {0, 0, 1}};
    *p++ = {{tr,         z}, {1, 1, 1}};
    *pp = p;
}

inline void Gen::sprite(Vertex **p, SpriteRenderer *x) {
    x->flags.clear(Renderer::Flag::UPDATED);
    const auto pos = x->pos.xy();
    const auto s = x->size / 2.0f;
    Gen::quad_vertices(p, pos - s, pos + s, -pos.y);
}

}

#endif
