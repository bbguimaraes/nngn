#ifndef NNGN_RENDER_RENDERERS_H
#define NNGN_RENDER_RENDERERS_H

#include "lua/table.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include "math/vec4.h"
#include "utils/def.h"
#include "utils/flags.h"

struct Entity;

namespace nngn {

struct Renderer {
    enum Type : u8 {
        SPRITE = 1,
        SCREEN_SPRITE,
        TRANSLUCENT,
        CUBE,
        VOXEL,
        N_TYPES,
    };
    enum Flag : u8 { UPDATED = 1u << 0 };
    Entity *entity = nullptr;
    vec3 pos = {};
    float z_off = 0;
    Flags<Flag> flags = {};
    bool updated() const { return this->flags.is_set(Flag::UPDATED); }
    void set_pos(vec3 p) { this->pos = p; this->flags |= Flag::UPDATED; }
};

struct SpriteRenderer : Renderer {
    vec2 size = {};
    std::array<vec2, 2> uv = {{{0, 1}, {1, 0}}};
    u32 tex = 0;
    static void uv_coords(
        uvec2 uv0, uvec2 uv1, uvec2 scale, std::span<float> s);
    template<typename T, std::size_t N>
    static std::span<float> uv_span(std::array<T, N> *a);
    void load(nngn::lua::table_view t);
};

struct CubeRenderer : Renderer {
    vec3 color = {1, 1, 1};
    float size = 0;
    void load(const nngn::lua::table &t);
};

struct VoxelRenderer : Renderer {
    std::array<vec4, 6> uv = {};
    vec3 size = {};
    u32 tex = 0;
    void load(const nngn::lua::table &t);
};

template<typename T, std::size_t N>
std::span<float> SpriteRenderer::uv_span(std::array<T, N> *a) {
    constexpr auto n = N * T::n_dim;
    static_assert(sizeof(*a) == n * sizeof(float));
    return {&(*a)[0][0], n};
}

inline void SpriteRenderer::uv_coords(
    uvec2 uv0, uvec2 uv1, uvec2 scale, std::span<float> s)
{
    assert(3 <= s.size());
    const auto fs = static_cast<vec2>(scale);
    const auto fc0 = static_cast<vec2>(uv0) / fs;
    const auto fc1 = static_cast<vec2>(uv1) / fs;
    s[0] = fc0.x, s[1] = 1 - fc0.y;
    s[2] = fc1.x, s[3] = 1 - fc1.y;
}

}

#endif
