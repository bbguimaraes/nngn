#include <algorithm>
#include <cstring>
#include <span>

#include <sol/table.hpp>

#include "entity.h"

#include "collision/collision.h"
#include "font/font.h"
#include "font/text.h"
#include "font/textbox.h"
#include "graphics/texture.h"
#include "timing/profile.h"
#include "utils/log.h"
#include "utils/vector.h"

#include "grid.h"
#include "light.h"
#include "map.h"
#include "render.h"

using nngn::u8, nngn::u32, nngn::u64;

namespace {

constexpr std::array<nngn::vec2, 2>
    CIRCLE_UV_32 = {{{ 32/512.0f, 1}, { 64/512.0f, 1 -  32/512.0f}}},
    CIRCLE_UV_64 = {{{128/512.0f, 1}, {256/512.0f, 1 - 128/512.0f}}};

template<std::size_t per_obj>
void update_indices(void*, void *p, u64 i, u64 n) {
    nngn::Renderers::gen_quad_idxs(
        i * per_obj / 6, n * per_obj / 6, static_cast<u32*>(p));
}

template<std::size_t per_obj>
void update_indices_with_base(const std::tuple<u64> *bi, u32 *p, u64 i, u64 n)
    { return update_indices<per_obj>({}, p, std::get<0>(*bi) + i, n); }

template<auto gen, typename VT, typename T>
bool write_to_buffer(
    nngn::Graphics *g, u32 b, u64 off, u64 n, u64 size, T *data
) {
    return g->write_to_buffer(
        b, off, n, size, data,
        [](void *data_, void *p, u64 i, u64 nw) {
            gen(static_cast<T*>(data_), static_cast<VT*>(p), i, nw);
        });
}

template<auto vgen, auto egen, typename T>
bool update_span(
    nngn::Graphics *g, std::span<T> s, u32 vbo, u32 ebo,
    std::size_t n_verts, std::size_t n_idxs
) {
    const auto n = s.size();
    if(!n)
        return g->set_buffer_size(ebo, 0), true;
    return g->update_buffers(
        vbo, ebo, 0, 0,
        n, n_verts * sizeof(nngn::Vertex),
        n, n_idxs * sizeof(u32), s.data(),
        [](void *d, void *vp, u64 i, u64 nw) {
            auto *p = static_cast<nngn::Vertex*>(vp);
            for(auto &x : std::span{
                static_cast<T*>(d) + i,
                static_cast<std::size_t>(nw)
            })
                vgen(&p, &x);
        }, egen);
}

template<auto vgen, auto egen, typename T>
bool update_with_state(
    nngn::Graphics *g, u32 vbo, u32 ebo, u64 voff, u64 eoff,
    u64 vn, u64 vsize, u64 en, u64 esize, T *data
) {
    const bool ok= write_to_buffer<vgen, nngn::Vertex>(
            g, vbo, voff, vn, vsize, data)
        && g->write_to_buffer(ebo, eoff, en, esize, {}, egen);
    if(!ok)
        return false;
    g->set_buffer_size(vbo, voff + vn * vsize);
    g->set_buffer_size(ebo, eoff + en * esize);
    return true;
}

void update_sprites_ortho(nngn::Vertex **p, nngn::SpriteRenderer *x) {
    x->flags.clear(nngn::Renderer::Flag::UPDATED);
    const auto pos = x->pos.xy();
    const auto s = x->size / 2.0f;
    nngn::Renderers::gen_quad_verts(
        p, pos - s, pos + s, -pos.y - x->z_off,
        {0, 0, 1}, x->tex, x->uv0, x->uv1);
}

void update_sprites_orthoz(nngn::Vertex **p, nngn::SpriteRenderer *x) {
    constexpr auto nl = nngn::Math::sq2_2<float>();
    x->flags.clear(nngn::Renderer::Flag::UPDATED);
    const auto pos = x->pos.xy();
    const auto s = x->size / 2.0f;
    nngn::Renderers::gen_quad_verts_zsprite(
        p, pos - s, pos + s, -(s.y + x->z_off),
        {0, -nl, nl}, x->tex, x->uv0, x->uv1);
}

void update_sprites_persp(nngn::Vertex **p, nngn::SpriteRenderer *x) {
    x->flags.clear(nngn::Renderer::Flag::UPDATED);
    const auto s = x->size / 2.0f;
    const nngn::vec2 bl = {x->pos.x - s.x, -s.y - x->z_off};
    nngn::Renderers::gen_quad_verts_persp(
        p, bl, {x->pos.x + s.x, bl.y + x->size.y},
        x->pos.y + x->z_off, {0, -1, 0}, x->tex, x->uv0, x->uv1);
}

void update_cubes_ortho(nngn::Vertex **p, nngn::CubeRenderer *x) {
    x->flags.clear(nngn::Renderer::Flag::UPDATED);
    nngn::Renderers::gen_cube_verts(
        p, {x->pos.x, x->pos.y, x->pos.z - x->pos.y},
        nngn::vec3{x->size}, x->color);
}

void update_cubes_persp(nngn::Vertex **p, nngn::CubeRenderer *x) {
    x->flags.clear(nngn::Renderer::Flag::UPDATED);
    nngn::Renderers::gen_cube_verts(p, x->pos, nngn::vec3{x->size}, x->color);
}

void update_voxels_ortho(nngn::Vertex **p, nngn::VoxelRenderer *x) {
    x->flags.clear(nngn::Renderer::Flag::UPDATED);
    nngn::Renderers::gen_cube_verts(
        p, {x->pos.x, x->pos.y, x->pos.z - x->pos.y},
        x->size, x->tex, x->uv);
}

void update_voxels_persp(nngn::Vertex **p, nngn::VoxelRenderer *x) {
    x->flags.clear(nngn::Renderer::Flag::UPDATED);
    nngn::Renderers::gen_cube_verts(p, x->pos, x->size, x->tex, x->uv);
}

void update_boxes(nngn::Vertex **p, nngn::SpriteRenderer *x) {
    constexpr nngn::vec3 norm = {0, 0, 1};
    const auto pos = x->pos.xy();
    auto s = x->size / 2.0f;
    nngn::vec2 bl = pos - s, tr = pos + s;
    nngn::Renderers::gen_quad_verts(p, bl, tr, 0, norm, {1, 1, 1});
    s = {0.5f, 0.5f};
    bl = pos - s;
    tr = pos + s;
    nngn::Renderers::gen_quad_verts(p, bl, tr, 0, norm, {1, 0, 0});
    bl.y = x->pos.y + x->z_off - s.y;
    tr.y = x->pos.y + x->z_off + s.y;
    nngn::Renderers::gen_quad_verts(p, bl, tr, 0, norm, {1, 0, 0});
}

void update_cube_dbg(nngn::Vertex **p, nngn::CubeRenderer *x) {
    nngn::Renderers::gen_cube_verts(p, x->pos, nngn::vec3{x->size}, {1, 1, 1});
}

void update_voxel_dbg(nngn::Vertex **p, nngn::VoxelRenderer *x) {
    nngn::Renderers::gen_cube_verts(p, x->pos, nngn::vec3{x->size}, {1, 1, 1});
}

constexpr u32 text_color(u8 r, u8 g, u8 b) {
    return (static_cast<u32>(r) << 16)
        | (static_cast<u32>(g) << 8)
        | static_cast<u32>(b);
}

float text_color(u32 c) {
    float ret = {};
    static_assert(sizeof(ret) == sizeof(c));
    std::memcpy(&ret, &c, sizeof(ret));
    return ret;
}

using UpdateTextData = std::tuple<
    const nngn::Font&, const nngn::Text&, float,
    nngn::vec2*, float*, u64*, u64*>;

void update_text(const UpdateTextData *data, nngn::Vertex *p, u64, u64 n) {
    using Command = nngn::Textbox::Command;
    constexpr auto
        RED = text_color(255, 32, 32),
        GREEN = text_color(32, 255, 32),
        BLUE = text_color(32, 32, 255);
    auto &[font, txt, left, pos_p, color_p, i_p, n_visible_p] = *data;
    auto pos = *pos_p;
    auto color = *color_p;
    auto i = *i_p;
    auto n_visible = *n_visible_p;
    const auto font_size = static_cast<float>(font.size);
    while(n--) {
        const auto c = static_cast<unsigned char>(
            txt.str[static_cast<std::size_t>(i++)]);
        switch(c) {
        case '\n': pos = {left, pos.y - font_size - txt.spacing}; continue;
        case Command::TEXT_WHITE: color = text_color(UINT32_MAX); continue;
        case Command::TEXT_RED: color = text_color(RED); continue;
        case Command::TEXT_GREEN: color = text_color(GREEN); continue;
        case Command::TEXT_BLUE: color = text_color(BLUE); continue;
        }
        const auto fc = font.chars[static_cast<std::size_t>(c)];
        const auto size = static_cast<nngn::vec2>(fc.size);
        const auto cpos = pos
            + nngn::vec2(font_size / 2, txt.size.y - font_size / 2)
            + static_cast<nngn::vec2>(fc.bearing);
        nngn::Renderers::gen_quad_verts(
            &p, cpos, cpos + size, color,
            {0, 0, 1}, static_cast<u32>(c),
            {0, size.y / font_size}, {size.x / font_size, 0});
        pos.x += fc.advance;
        ++n_visible;
    }
    *pos_p = pos;
    *color_p = color;
    *i_p = i;
    *n_visible_p = n_visible;
}

void update_textbox(void *data, void *vp, u64, u64) {
    constexpr nngn::vec3 norm = {0, 0, 1};
    auto *p = static_cast<nngn::Vertex*>(vp);
    const auto *const t = static_cast<const nngn::Textbox*>(data);
    nngn::Renderers::gen_quad_verts(
        &p, t->title_bl, t->title_tr, 0, norm, {0, 0, 1});
    nngn::Renderers::gen_quad_verts(
        &p, t->str_bl, t->str_tr, 0, norm, {1, 1, 1});
}

using UpdateSelectionData = std::tuple<
    std::unordered_set<const nngn::Renderer*>::const_iterator*,
    std::span<const nngn::SpriteRenderer>>;

void update_selections(
    const UpdateSelectionData *data, nngn::Vertex *p, u64, u64 n
) {
    auto &[it_p, sprites] = *data;
    const auto is_sprite = [b = &*begin(sprites), e = &*end(sprites)](auto *x)
        { return b <= x && x < e; };
    auto it = *it_p;
    while(n--) {
        const auto &x = **it++;
        if(!is_sprite(&x))
            continue;
        const auto &dx = static_cast<const nngn::SpriteRenderer&>(x);
        const auto s = dx.size / 2.0f;
        nngn::Renderers::gen_quad_verts(
            &p, {dx.pos.x - s.x, dx.pos.y - s.y},
            {dx.pos.x + s.x, dx.pos.y + s.y},
            0, {0, 0, 1}, {1, 1, 0});
    }
    *it_p = it;
}

using UpdateAABBData = std::tuple<
    const nngn::AABBCollider*,
    const std::unordered_set<const nngn::Collider*>&>;

void update_aabbs(const UpdateAABBData *data, nngn::Vertex *p, u64 i, u64 n) {
    auto &[aabbs, lookup] = *data;
    for(; n--; ++i) {
        const auto &x = aabbs[i];
        nngn::Renderers::gen_quad_verts(
            &p, x.bl, x.tr, 0, {0, 0, 1},
            lookup.find(&x) != cend(lookup)
                ? nngn::vec3(0, 1, 0) : nngn::vec3(1, 0, 0));
    }
}

void update_aabb_circles(
    nngn::Vertex **p, const nngn::AABBCollider *x
) {
    const auto uv = x->radius < 32.0f ? CIRCLE_UV_32 : CIRCLE_UV_64;
    nngn::Renderers::gen_quad_verts(
        p, x->center - x->radius, x->center + x->radius, 0, {0, 0, 1}, 1,
        uv[0], uv[1]);
}

using UpdateBBData = std::tuple<
    const nngn::BBCollider*,
    const std::unordered_set<const nngn::Collider*>&>;

void update_bbs(const UpdateBBData *data, nngn::Vertex *p, u64 i, u64 n) {
    auto &[bbs, lookup] = *data;
    constexpr nngn::vec3 norm = {0, 0, 1};
    for(; n--; ++i) {
        const auto &x = bbs[i];
        const auto color = lookup.find(&x) != cend(lookup)
            ? nngn::vec3(0, 1, 0) : nngn::vec3(1, 0, 0);
        std::array<nngn::vec2, 4> rot = {{
            {x.bl.x, x.bl.y},
            {x.tr.x, x.bl.y},
            {x.bl.x, x.tr.y},
            {x.tr.x, x.tr.y}}};
        const auto pos = x.bl + (x.tr - x.bl) / 2.0f;
        for(auto &r : rot) {
            r -= pos;
            r = {
                r.x * x.cos - r.y * x.sin,
                r.x * x.sin + r.y * x.cos};
            r += pos;
        }
        *p++ = {{rot[0], 0}, norm, color};
        *p++ = {{rot[1], 0}, norm, color};
        *p++ = {{rot[2], 0}, norm, color};
        *p++ = {{rot[3], 0}, norm, color};
    }
}

void update_spheres(
    nngn::Vertex **p, const nngn::SphereCollider *x
) {
    const auto uv = x->r / 2.0f < 32.0f ? CIRCLE_UV_32 : CIRCLE_UV_64;
    nngn::Renderers::gen_quad_verts(
        p, (x->pos - x->r).xy(), (x->pos + x->r).xy(), 0, {0, 0, 1}, 1,
        uv[0], uv[1]);
}

using UpdateLightsData = std::tuple<const nngn::Light*, float>;

template<auto vgen>
bool update_lights(
    nngn::Graphics *g, u32 vbo, u32 ebo,
    u64 *voff, u64 *eoff, u64 *ei,
    std::span<const nngn::Light> s, float shadow_map_far
) {
    constexpr auto vsize = 6 * 4 * sizeof(nngn::Vertex);
    constexpr auto esize = 6 * 6 * sizeof(u32);
    const auto vo = std::exchange(*voff, *voff + s.size() * vsize);
    const auto eo = std::exchange(*eoff, *eoff + s.size() * esize);
    const auto cur_ei = std::exchange(*ei, *ei + s.size());
    return write_to_buffer<vgen, nngn::Vertex>(
            g, vbo, vo, s.size(), vsize,
            nngn::rptr(UpdateLightsData{s.data(), shadow_map_far}))
        && write_to_buffer<update_indices_with_base<6 * 6>, u32>(
            g, ebo, eo, s.size(), esize, nngn::rptr(std::tuple{cur_ei}));
}

void update_dir_lights(
    const UpdateLightsData *data, nngn::Vertex *p, u64 i, u64 n
) {
    auto [lights, shadow_map_far] = *data;
    for(n += i; i < n; ++i) {
        const auto &x = lights[i];
        nngn::Renderers::gen_cube_verts(
            &p, x.ortho_view_pos(shadow_map_far), nngn::vec3{8}, x.color.xyz());
    }
}

void update_point_lights(
    const UpdateLightsData *data, nngn::Vertex *p, u64 i, u64 n
) {
    auto [lights, _] = *data;
    for(n += i; i < n; ++i) {
        const auto &x = lights[i];
        nngn::Renderers::gen_cube_verts(
            &p, x.pos, nngn::vec3{8}, x.color.xyz());
    }
}

void update_ranges(nngn::Vertex **p, const nngn::Light *x) {
    nngn::Renderers::gen_cube_verts(
        p, x->pos, nngn::vec3{.2f * x->range()}, x->color.xyz());
}

bool update_shadow_maps(
    nngn::Graphics *g, u32 vbo, u32 ebo,
    nngn::uvec2 offset, nngn::uvec2 size
) {
    using data = std::tuple<nngn::uvec2, nngn::uvec2>;
    constexpr auto vgen = [](
        void *d, void *vp, u64 y, u64 nw
    ) {
        constexpr nngn::vec2 ext(128), pad(ext / 8.0f), comb = ext + pad;
        auto *p = static_cast<nngn::Vertex*>(vp);
        auto [off, size_] = *static_cast<data*>(d);
        const auto off_f = static_cast<nngn::vec2>(off);
        const auto base_pos = pad
            + comb * off_f.xy()
            + comb.y * static_cast<float>(y);
        auto pos = base_pos;
        auto tex = static_cast<u32>(y * size_.x);
        for(const auto e = y + nw; y < e; ++y, pos += comb.y)
            for(std::size_t x = 0; x < size_.x; ++x, ++tex) {
                pos.x = base_pos.x + static_cast<float>(x) * comb.x;
                nngn::Renderers::gen_quad_verts(&p,
                    pos, pos + ext, 0, {0, 0, 1},
                    tex, {0, 1}, {1, 0});
            }
    };
    const auto n = nngn::Math::product(size);
    const bool ok = g->write_to_buffer(
            vbo, 0, size.y, 4 * size.x * sizeof(nngn::Vertex),
            rptr(data{offset, size}), vgen)
        && g->write_to_buffer(
            ebo, 0, n, 6 * sizeof(u32), {}, update_indices<6>);
    if(!ok)
        return false;
    g->set_buffer_size(vbo, n * 4 * sizeof(nngn::Vertex));
    g->set_buffer_size(ebo, n * 6 * sizeof(u32));;
    return true;
}

}

namespace nngn {

void Renderers::init(
    Textures *t, const Fonts *f, const Textbox *tb, const Grid *g,
    const Colliders *c, const Lighting *l, const Map *m
) {
    this->textures = t;
    this->fonts = f;
    this->textbox = tb;
    this->grid = g;
    this->colliders = c;
    this->lighting = l;
    this->map = m;
}

std::size_t Renderers::n() const {
    return this->sprites.size()
        + this->translucent.size()
        + this->cubes.size()
        + this->voxels.size();
}

bool Renderers::set_max_sprites(std::size_t n) {
    set_capacity(&this->sprites, n);
    const auto vsize = 4 * n * sizeof(Vertex);
    const auto esize = 6 * n * sizeof(u32);
    return this->graphics->set_buffer_capacity(this->sprite_vbo, vsize)
        && this->graphics->set_buffer_capacity(this->sprite_ebo, esize)
        && this->graphics->set_buffer_capacity(this->box_vbo, 3 * vsize)
        && this->graphics->set_buffer_capacity(this->box_ebo, 3 * esize);
}

bool Renderers::set_max_translucent(std::size_t n) {
    set_capacity(&this->translucent, n);
    return this->graphics->set_buffer_capacity(
            this->translucent_vbo, 4 * n * sizeof(Vertex))
        && this->graphics->set_buffer_capacity(
            this->translucent_ebo, 6 * n * sizeof(u32));
}

bool Renderers::set_max_cubes(std::size_t n) {
    set_capacity(&this->cubes, n);
    const u64 vsize = 24 * n * sizeof(Vertex);
    const u64 esize = 36 * n * sizeof(std::uint32_t);
    return this->graphics->set_buffer_capacity(this->cube_vbo, vsize)
        && this->graphics->set_buffer_capacity(this->cube_ebo, esize)
        && this->graphics->set_buffer_capacity(this->cube_debug_vbo, vsize)
        && this->graphics->set_buffer_capacity(this->cube_debug_ebo, esize);
}

bool Renderers::set_max_voxels(std::size_t n) {
    set_capacity(&this->voxels, n);
    const u64 vsize = 24 * n * sizeof(Vertex);
    const u64 esize = 36 * n * sizeof(std::uint32_t);
    return this->graphics->set_buffer_capacity(this->voxel_vbo, vsize)
        && this->graphics->set_buffer_capacity(this->voxel_ebo, esize)
        && this->graphics->set_buffer_capacity(this->voxel_debug_vbo, vsize)
        && this->graphics->set_buffer_capacity(this->voxel_debug_ebo, esize);
}

bool Renderers::set_max_text(std::size_t n) {
    return this->graphics->set_buffer_capacity(
            this->text_vbo, 4 * n * sizeof(Vertex))
        && this->graphics->set_buffer_capacity(
            this->text_ebo, 6 * n * sizeof(u32));
}

bool Renderers::set_max_colliders(std::size_t n) {
    if(!this->graphics)
        return true;
    const auto vsize = 4 * n * sizeof(Vertex);
    const auto esize = 6 * n * sizeof(u32);
    return this->graphics->set_buffer_capacity(this->aabb_vbo, vsize)
        && this->graphics->set_buffer_capacity(this->aabb_ebo, esize)
        && this->graphics->set_buffer_capacity(this->aabb_circle_vbo, vsize)
        && this->graphics->set_buffer_capacity(this->aabb_circle_ebo, esize)
        && this->graphics->set_buffer_capacity(this->bb_vbo, vsize)
        && this->graphics->set_buffer_capacity(this->bb_ebo, esize)
        && this->graphics->set_buffer_capacity(this->bb_circle_vbo, vsize)
        && this->graphics->set_buffer_capacity(this->bb_circle_ebo, esize)
        && this->graphics->set_buffer_capacity(this->sphere_coll_vbo, vsize)
        && this->graphics->set_buffer_capacity(this->sphere_coll_ebo, esize);
}

void Renderers::set_debug(std::underlying_type_t<Debug> d) {
    auto old = this->m_debug;
    this->m_debug = {d};
    if((old ^ this->m_debug) & Debug::RECT)
        this->flags |= Flag::RECT_UPDATED;
}

void Renderers::set_perspective(bool p) {
    constexpr auto f =
        Flag::SPRITES_UPDATED
        | Flag::TRANSLUCENT_UPDATED
        | Flag::CUBES_UPDATED;
    this->flags.set(Flag::PERSPECTIVE, p);
    this->flags.set(f);
}

void Renderers::set_zsprites(bool z) {
    constexpr auto f =
        Flag::SPRITES_UPDATED
        | Flag::TRANSLUCENT_UPDATED
        | Flag::CUBES_UPDATED;
    const bool old = this->flags.is_set(Flag::ZSPRITES);
    this->flags.set(Flag::ZSPRITES, z);
    if(old != z)
        this->flags.set(f);
}

bool Renderers::set_graphics(Graphics *g) {
    constexpr u32
        TRIANGLE_MAX = 3,
        TRIANGLE_VBO_SIZE = TRIANGLE_MAX * sizeof(Vertex),
        TRIANGLE_EBO_SIZE = TRIANGLE_MAX * sizeof(u32),
        SPHERE_MAX = 1u << 16,
        TEXTBOX_MAX = 1,
        SELECTION_MAX = 1024,
        LIGHTS_MAX = 2 * NNGN_MAX_LIGHTS,
        RANGE_MAX = NNGN_MAX_LIGHTS,
        DEPTH_MAX = NNGN_MAX_LIGHTS,
        DEPTH_CUBE_MAX = 6 * NNGN_MAX_LIGHTS;
    using Pipeline = Graphics::PipelineConfiguration;
    using Stage = Graphics::RenderList::Stage;
    using BufferPair = std::pair<u32, u32>;
    constexpr auto vertex = Graphics::BufferConfiguration::Type::VERTEX;
    constexpr auto index = Graphics::BufferConfiguration::Type::INDEX;
    this->graphics = g;
    u32
        triangle_pipeline = {}, sprite_pipeline = {}, voxel_pipeline = {},
        box_pipeline = {}, font_pipeline = {}, line_pipeline = {},
        circle_pipeline = {},
        triangle_depth_pipeline = {}, sprite_depth_pipeline = {},
        triangle_vbo = {}, triangle_ebo = {};
    return (triangle_pipeline = g->create_pipeline({
            .name = "triangle_pipeline",
            .type = Pipeline::Type::TRIANGLE,
            .flags = static_cast<Pipeline::Flag>(
                Pipeline::Flag::DEPTH_TEST
                | Pipeline::Flag::DEPTH_WRITE
                | Pipeline::Flag::CULL_BACK_FACES),
        }))
        && (sprite_pipeline = g->create_pipeline({
            .name = "sprite_pipeline",
            .type = Pipeline::Type::SPRITE,
            .flags = static_cast<Pipeline::Flag>(
                Pipeline::Flag::DEPTH_TEST
                | Pipeline::Flag::DEPTH_WRITE),
        }))
        && (voxel_pipeline = g->create_pipeline({
            .name = "voxel_pipeline",
            .type = Pipeline::Type::VOXEL,
            .flags = static_cast<Pipeline::Flag>(
                Pipeline::Flag::DEPTH_TEST
                | Pipeline::Flag::DEPTH_WRITE
                | Pipeline::Flag::CULL_BACK_FACES),
        }))
        && (box_pipeline = g->create_pipeline({
            .name = "box_pipeline",
            .type = Pipeline::Type::TRIANGLE,
        }))
        && (font_pipeline = g->create_pipeline({
            .name = "font_pipeline",
            .type = Pipeline::Type::FONT,
        }))
        && (line_pipeline = g->create_pipeline({
            .name = "line_pipeline",
            .type = Pipeline::Type::TRIANGLE,
            .flags = Pipeline::Flag::LINE,
        }))
        && (circle_pipeline = g->create_pipeline({
            .name = "circle_pipeline",
            .type = Pipeline::Type::SPRITE,
        }))
        && (triangle_depth_pipeline = g->create_pipeline({
            .name = "triangle_depth_pipeline",
            .type = Pipeline::Type::TRIANGLE_DEPTH,
            .flags = static_cast<Pipeline::Flag>(
                Pipeline::Flag::DEPTH_TEST
                | Pipeline::Flag::DEPTH_WRITE
                | Pipeline::Flag::CULL_BACK_FACES),
        }))
        && (sprite_depth_pipeline = g->create_pipeline({
            .name = "sprite_depth_pipeline",
            .type = Pipeline::Type::SPRITE_DEPTH,
            .flags = static_cast<Pipeline::Flag>(
                Pipeline::Flag::DEPTH_TEST
                | Pipeline::Flag::DEPTH_WRITE),
        }))
        && (triangle_vbo = g->create_buffer({
            .name = "triangle_vbo",
            .type = vertex,
            .size = TRIANGLE_VBO_SIZE,
        }))
        && (triangle_ebo = g->create_buffer({
            .name = "triangle_ebo",
            .type = index,
            .size = TRIANGLE_EBO_SIZE,
        }))
        && (this->translucent_vbo = g->create_buffer({
            .name = "translucent_vbo",
            .type = vertex,
        }))
        && (this->translucent_ebo = g->create_buffer({
            .name = "translucent_ebo",
            .type = index,
        }))
        && (this->sprite_vbo = g->create_buffer({
            .name = "sprite_vbo",
            .type = vertex,
        }))
        && (this->sprite_ebo = g->create_buffer({
            .name = "sprite_ebo",
            .type = index,
        }))
        && (this->sphere_vbo = g->create_buffer({
            .name = "sphere_vbo",
            .type = vertex,
            .size = 24 * SPHERE_MAX * sizeof(Vertex),
        }))
        && (this->sphere_ebo = g->create_buffer({
            .name = "sphere_ebo",
            .type = index,
            .size = 36 * SPHERE_MAX * sizeof(u32),
        }))
        && (this->box_vbo = g->create_buffer({
            .name = "box_vbo",
            .type = vertex,
        }))
        && (this->box_ebo = g->create_buffer({
            .name = "box_ebo",
            .type = index,
        }))
        && (this->cube_vbo = g->create_buffer({
            .name = "cube_vbo",
            .type = vertex,
        }))
        && (this->cube_ebo = g->create_buffer({
            .name = "cube_ebo",
            .type = index,
        }))
        && (this->cube_debug_vbo = g->create_buffer({
            .name = "cube_debug_vbo",
            .type = vertex,
        }))
        && (this->cube_debug_ebo = g->create_buffer({
            .name = "cube_debug_ebo",
            .type = index,
        }))
        && (this->voxel_vbo = g->create_buffer({
            .name = "voxel_vbo",
            .type = vertex,
        }))
        && (this->voxel_ebo = g->create_buffer({
            .name = "voxel_ebo",
            .type = index,
        }))
        && (this->voxel_debug_vbo = g->create_buffer({
            .name = "voxel_debug_vbo",
            .type = vertex,
        }))
        && (this->voxel_debug_ebo = g->create_buffer({
            .name = "voxel_debug_ebo",
            .type = index,
        }))
        && (this->text_vbo = g->create_buffer({
            .name = "text_vbo",
            .type = vertex,
        }))
        && (this->text_ebo = g->create_buffer({
            .name = "text_ebo",
            .type = index,
        }))
        && (this->textbox_vbo = g->create_buffer({
            .name = "textbox_vbo",
            .type = vertex,
            .size = 2 * 4 * TEXTBOX_MAX * sizeof(Vertex),
        }))
        && (this->textbox_ebo = g->create_buffer({
            .name = "textbox_ebo",
            .type = index,
            .size = 2 * 6 * TEXTBOX_MAX * sizeof(u32),
        }))
        && (this->selection_vbo = g->create_buffer({
            .name = "selection_vbo",
            .type = vertex,
            .size = 4 * SELECTION_MAX * sizeof(Vertex),
        }))
        && (this->selection_ebo = g->create_buffer({
            .name = "selection_ebo",
            .type = index,
            .size = 6 * SELECTION_MAX * sizeof(u32),
        }))
        && (this->aabb_vbo = g->create_buffer({
            .name = "aabb_vbo",
            .type = vertex,
        }))
        && (this->aabb_ebo = g->create_buffer({
            .name = "aabb_ebo",
            .type = index,
        }))
        && (this->aabb_circle_vbo = g->create_buffer({
            .name = "aabb_circle_vbo",
            .type = vertex,
        }))
        && (this->aabb_circle_ebo = g->create_buffer({
            .name = "aabb_circle_ebo",
            .type = index,
        }))
        && (this->bb_vbo = g->create_buffer({
            .name = "bb_vbo",
            .type = vertex,
        }))
        && (this->bb_ebo = g->create_buffer({
            .name = "bb_ebo",
            .type = index,
        }))
        && (this->bb_circle_vbo = g->create_buffer({
            .name = "bb_circle_vbo",
            .type = vertex,
        }))
        && (this->bb_circle_ebo = g->create_buffer({
            .name = "bb_circle_ebo",
            .type = index,
        }))
        && (this->sphere_coll_vbo = g->create_buffer({
            .name = "sphere_coll_vbo",
            .type = vertex,
        }))
        && (this->sphere_coll_ebo = g->create_buffer({
            .name = "sphere_coll_ebo",
            .type = index,
        }))
        && (this->lights_vbo = g->create_buffer({
            .name = "lights_vbo",
            .type = vertex,
            .size = 24 * LIGHTS_MAX * sizeof(Vertex),
        }))
        && (this->lights_ebo = g->create_buffer({
            .name = "lights_ebo",
            .type = index,
            .size = 36 * LIGHTS_MAX * sizeof(u32),
        }))
        && (this->range_vbo = g->create_buffer({
            .name = "range_vbo",
            .type = vertex,
            .size = 24 * RANGE_MAX * sizeof(Vertex),
        }))
        && (this->range_ebo = g->create_buffer({
            .name = "range_ebo",
            .type = index,
            .size = 36 * RANGE_MAX * sizeof(u32),
        }))
        && (this->depth_vbo = g->create_buffer({
            .name = "depth_vbo",
            .type = vertex,
            .size = 4 * DEPTH_MAX * sizeof(Vertex),
        }))
        && (this->depth_ebo = g->create_buffer({
            .name = "depth_ebo",
            .type = index,
            .size = 6 * DEPTH_MAX * sizeof(u32),
        }))
        && (this->depth_cube_vbo = g->create_buffer({
            .name = "depth_cube_vbo",
            .type = vertex,
            .size = 4 * DEPTH_CUBE_MAX * sizeof(Vertex),
        }))
        && (this->depth_cube_ebo = g->create_buffer({
            .name = "depth_cube_ebo",
            .type = index,
            .size = 6 * DEPTH_CUBE_MAX * sizeof(u32),
        }))
        && g->update_buffers(
            triangle_vbo, triangle_ebo, 0, 0,
            1, TRIANGLE_VBO_SIZE, 1, TRIANGLE_EBO_SIZE, nullptr,
            [](void*, void *p, u64, u64) {
                const auto v = std::array<nngn::Vertex, TRIANGLE_MAX>{{
                    {{24, 40, 0}, {0, 0, 1}, {1, 0, 0}},
                    {{ 8,  8, 0}, {0, 0, 1}, {0, 1, 0}},
                    {{40,  8, 0}, {0, 0, 1}, {0, 0, 1}}}};
                memcpy(p, std::span{v});
            },
            [](void*, void *p, u64, u64) {
                const auto v = std::array<u32, TRIANGLE_MAX>{0, 1, 2};
                memcpy(p, std::span{v});
            })
        && g->set_render_list(Graphics::RenderList{
            .depth = std::to_array<Stage>({{
                .pipeline = sprite_depth_pipeline,
                .buffers = std::to_array<BufferPair>({
                    {this->map->vbo(), this->map->ebo()},
                    {this->sprite_vbo, this->sprite_ebo},
                }),
            }, {
                .pipeline = triangle_depth_pipeline,
                .buffers = std::to_array<BufferPair>({
                    {this->voxel_vbo, this->voxel_ebo},
                    {this->cube_vbo, this->cube_ebo},
                }),
            }}),
            .map_ortho = std::to_array<Stage>({{
                .pipeline = circle_pipeline,
                .buffers = std::to_array<BufferPair>({
                    {this->map->vbo(), this->map->ebo()},
                }),
            }}),
            .map_persp = std::to_array<Stage>({{
                .pipeline = sprite_pipeline,
                .buffers = std::to_array<BufferPair>({
                    {this->map->vbo(), this->map->ebo()},
                }),
            }}),
            .normal = std::to_array<Stage>({{
                .pipeline = voxel_pipeline,
                .buffers = std::to_array<BufferPair>({
                    {this->voxel_vbo, this->voxel_ebo},
                }),
            }, {
                .pipeline = triangle_pipeline,
                .buffers = std::to_array<BufferPair>({
                    {this->cube_vbo, this->cube_ebo},
                }),
            }, {
                .pipeline = sprite_pipeline,
                .buffers = std::to_array<BufferPair>({
                    {this->sprite_vbo, this->sprite_ebo},
                }),
            }}),
            .no_light = std::to_array<Stage>({{
                .pipeline = sprite_pipeline,
                .buffers = std::to_array<BufferPair>({
                    {this->translucent_vbo, this->translucent_ebo},
                }),
            }}),
            .overlay = std::to_array<Stage>({{
                .pipeline = circle_pipeline,
                .buffers = std::to_array<BufferPair>({
                    {this->aabb_circle_vbo, this->aabb_circle_ebo},
                    {this->bb_circle_vbo, this->bb_circle_ebo},
                    {this->sphere_vbo, this->sphere_ebo},
                }),
            }, {
                .pipeline = box_pipeline,
                .buffers = std::to_array<BufferPair>({
                    {this->box_vbo, this->box_ebo},
                    {this->cube_debug_vbo, this->cube_debug_ebo},
                    {this->voxel_debug_vbo, this->voxel_debug_ebo},
                    {this->selection_vbo, this->selection_ebo},
                    {this->aabb_vbo, this->aabb_ebo},
                    {this->bb_vbo, this->bb_ebo},
                    {this->lights_vbo, this->lights_ebo},
                }),
            }, {
                .pipeline = line_pipeline,
                .buffers = std::to_array<BufferPair>({
                    {this->grid->vbo(), this->grid->ebo()},
                    {this->range_vbo, this->range_ebo},
                }),
            }}),
            .hud = std::to_array<Stage>({{
                .pipeline = triangle_pipeline,
                .buffers = std::to_array<BufferPair>({
                    {triangle_vbo, triangle_ebo},
                }),
            }, {
                .pipeline = box_pipeline,
                .buffers = std::to_array<BufferPair>({
                    {this->textbox_vbo, this->textbox_ebo},
                }),
            }, {
                .pipeline = font_pipeline,
                .buffers = std::to_array<BufferPair>({
                    {this->text_vbo, this->text_ebo},
                }),
            }}),
            .shadow_maps = std::to_array<Stage>({{
                .pipeline = circle_pipeline,
                .buffers = std::to_array<BufferPair>({
                    {this->depth_vbo, this->depth_ebo},
                }),
            }}),
            .shadow_cubes = std::to_array<Stage>({{
                .pipeline = circle_pipeline,
                .buffers = std::to_array<BufferPair>({
                    {this->depth_cube_vbo, this->depth_cube_ebo},
                }),
            }}),
        });
}

Renderer *Renderers::load(const sol::stack_table &t) {
    const auto load = [&t](auto &v, const char *name) {
        if(v.size() == v.capacity()) {
            Log::l()
                << "Renderers::load: cannot add more "
                << name << " renderers" << std::endl;
            return static_cast<decltype(v.data())>(nullptr);
        }
        auto x = &v.emplace_back();
        x->load(t);
        x->flags |= Renderer::Flag::UPDATED;
        return x;
    };
    const auto load_tex = [this, &load](auto &v, const char *name) {
        auto ret = load(v, name);
        if(ret) {
            assert(this->textures);
            if(ret->tex)
                this->textures->add_ref(ret->tex);
        }
        return ret;
    };
    switch(const Renderer::Type type = t["type"]) {
    case Renderer::Type::SPRITE: return load_tex(this->sprites, "sprite");
    case Renderer::Type::TRANSLUCENT: return load_tex(this->translucent, "translucent");
    case Renderer::Type::CUBE: return load(this->cubes, "cube");
    case Renderer::Type::VOXEL: return load(this->voxels, "voxel");
    case Renderer::Type::N_TYPES:
    default:
        Log::l() << "invalid type: " << static_cast<int>(type) << '\n';
        return nullptr;
    }
}

void Renderers::remove(Renderer *p) {
    const auto is_in = [p](const auto &v)
        { return v.data() <= p && p < v.data() + v.size(); };
    const auto remove = [this, p](auto *v, auto update_flag) {
        using T = typename std::decay_t<decltype(*v)>::pointer;
        if(auto i = vector_linear_erase(v, static_cast<T>(p)); i != end(*v)) {
            i->entity->renderer = &*i;
            i->flags |= Renderer::Flag::UPDATED;
        }
        this->flags |= update_flag;
    };
    const auto remove_tex = [this, &remove, p](auto *v, auto update_flag) {
        using T = typename std::decay_t<decltype(*v)>::const_pointer;
        if(const auto t = static_cast<T>(p)->tex)
            this->textures->remove(t);
        remove(v, update_flag);
    };
    if(is_in(this->sprites))
        remove_tex(&this->sprites, Flag::SPRITES_UPDATED);
    if(is_in(this->translucent))
        remove_tex(&this->translucent, Flag::TRANSLUCENT_UPDATED);
    if(is_in(this->cubes))
        remove(&this->cubes, Flag::CUBES_UPDATED);
    if(is_in(this->voxels))
        remove(&this->voxels, Flag::VOXELS_UPDATED);
    auto i = this->selections.find(p);
    if(i != this->selections.end())
        this->selections.erase(i);
}

void Renderers::add_selection(const Renderer *r) {
    this->selections.insert(r);
    this->flags |= Flag::SELECTION_UPDATED;
}

void Renderers::remove_selection(const Renderer *r) {
    this->selections.erase(r);
    this->flags |= Flag::SELECTION_UPDATED;
}

bool Renderers::update() {
    NNGN_LOG_CONTEXT_CF(Renderers);
    const auto update_sprites = [this] {
        NNGN_LOG_CONTEXT("sprites");
        const auto vbo = this->sprite_vbo;
        const auto ebo = this->sprite_ebo;
        if(this->flags.is_set(Flag::ZSPRITES))
            return update_span<::update_sprites_orthoz, update_indices<6>>(
                this->graphics, std::span{this->sprites}, vbo, ebo, 4, 6);
        if(this->flags.is_set(Flag::PERSPECTIVE))
            return update_span<::update_sprites_persp, update_indices<6>>(
                this->graphics, std::span{this->sprites}, vbo, ebo, 4, 6);
        return update_span<::update_sprites_ortho, update_indices<6>>(
            this->graphics, std::span{this->sprites}, vbo, ebo, 4, 6);
    };
    const auto update_translucent = [this] {
        NNGN_LOG_CONTEXT("translucent");
        const auto vbo = this->translucent_vbo;
        const auto ebo = this->translucent_ebo;
        if(this->flags.is_set(Flag::ZSPRITES))
            return update_span<::update_sprites_orthoz, update_indices<6>>(
                this->graphics, std::span{this->translucent}, vbo, ebo, 4, 6);
        if(this->flags.is_set(Flag::PERSPECTIVE))
            return update_span<::update_sprites_persp, update_indices<6>>(
                this->graphics, std::span{this->translucent}, vbo, ebo, 4, 6);
        return update_span<::update_sprites_ortho, update_indices<6>>(
            this->graphics, std::span{this->translucent}, vbo, ebo, 4, 6);
    };
    const auto update_cubes = [this] {
        NNGN_LOG_CONTEXT("cube");
        const auto vbo = this->cube_vbo;
        const auto ebo = this->cube_ebo;
        return this->flags.is_set(Flag::PERSPECTIVE)
            ? update_span<::update_cubes_persp, update_indices<6 * 6>>(
                this->graphics, std::span{this->cubes}, vbo, ebo, 6 * 4, 6 * 6)
            : update_span<::update_cubes_ortho, update_indices<6 * 6>>(
                this->graphics, std::span{this->cubes}, vbo, ebo, 6 * 4, 6 * 6);
    };
    const auto update_rect = [this] {
        NNGN_LOG_CONTEXT("rect");
        return update_span<::update_boxes, update_indices<3 * 6>>(
            this->graphics, std::span{this->sprites},
            this->box_vbo, this->box_ebo, 3 * 4, 3 * 6);
    };
    const auto update_cube_dbg = [this] {
        NNGN_LOG_CONTEXT("cubes debug");
        return update_span<::update_cube_dbg, update_indices<6 * 6>>(
            this->graphics, std::span{this->cubes},
            this->cube_debug_vbo, this->cube_debug_ebo, 6 * 4, 6 * 6);
    };
    const auto update_voxels = [this] {
        NNGN_LOG_CONTEXT("voxel");
        const auto vbo = this->voxel_vbo;
        const auto ebo = this->voxel_ebo;
        return this->flags.is_set(Flag::PERSPECTIVE)
            ? update_span<::update_voxels_persp, update_indices<6 * 6>>(
                this->graphics, std::span{this->voxels},
                vbo, ebo, 6 * 4, 6 * 6)
            : update_span<::update_voxels_ortho, update_indices<6 * 6>>(
                this->graphics, std::span{this->voxels},
                vbo, ebo, 6 * 4, 6 * 6);
    };
    const auto update_voxel_dbg = [this] {
        NNGN_LOG_CONTEXT("voxels debug");
        return update_span<::update_voxel_dbg, update_indices<6 * 6>>(
            this->graphics, std::span{this->voxels},
            this->voxel_debug_vbo, this->voxel_debug_ebo, 6 * 4, 6 * 6);
    };
    const auto update_text = [this] {
        NNGN_LOG_CONTEXT("text");
        if(!this->textbox->updated())
            return true;
        const auto vbo = this->text_vbo;
        const auto ebo = this->text_ebo;
        const auto n_fonts = this->fonts->n();
        if(n_fonts < 2) {
            this->graphics->set_buffer_size(ebo, 0);
            return true;
        }
        const auto n = this->textbox->text_length();
        if(!n) {
            this->graphics->set_buffer_size(ebo, 0);
            return true;
        }
        const auto render = [this, vbo, ebo](
            const auto &f, const auto &txt,
            auto pos, auto *voff, auto *eoff, auto *ei
        ) {
            if(!txt.cur)
                return true;
            constexpr auto vsize = 4 * sizeof(Vertex);
            constexpr auto esize = 6 * sizeof(u32);
            const auto vbytes = vsize * txt.cur;
            const auto vo = std::exchange(*voff, *voff + vbytes);
            u64 n_visible = 0;
            const bool ok = write_to_buffer<::update_text, Vertex>(
                this->graphics, vbo, vo, txt.cur, vsize,
                rptr(UpdateTextData{
                    std::ref(f), std::ref(txt), pos.x, &pos,
                    rptr(text_color(UINT32_MAX)), rptr(u64{}), &n_visible}));
            if(!ok)
                return false;
            const auto ebytes = esize * n_visible;
            const auto eo = std::exchange(*eoff, *eoff + ebytes);
            const auto cur_ei = std::exchange(*ei, *ei + txt.cur);
            return write_to_buffer<update_indices_with_base<6>, u32>(
                this->graphics, ebo, eo, txt.cur, esize,
                rptr(std::tuple{cur_ei}));
        };
        const auto &t = *this->textbox;
        const auto &f = this->fonts->fonts()[n_fonts - 1];
        u64 ei = {};
        u64 voff = {}, eoff = {};
        const bool ok = render(f, t.title, t.title_bl, &voff, &eoff, &ei)
            && render(f, t.str, t.str_bl, &voff, &eoff, &ei);
        if(!ok)
            return false;
        this->graphics->set_buffer_size(vbo, voff);
        this->graphics->set_buffer_size(ebo, eoff);
        return true;
    };
    const auto update_textbox = [this] {
        NNGN_LOG_CONTEXT("textbox");
        if(!this->textbox->updated())
            return true;
        const auto vbo = this->textbox_vbo;
        const auto ebo = this->textbox_ebo;
        if(this->fonts->n() < 2 || this->textbox->str.str.empty()) {
            this->graphics->set_buffer_size(ebo, 0);
            return true;
        }
        return this->graphics->update_buffers(
            vbo, ebo, 0, 0,
            1, 2 * 4 * sizeof(Vertex), 1, 2 * 6 * sizeof(std::uint32_t),
            const_cast<void*>(static_cast<const void*>(this->textbox)),
            ::update_textbox,
            [](void *d, void *p, auto...) { update_indices<6>(d, p, 0, 2); });
    };
    const auto update_selections = [this] {
        NNGN_LOG_CONTEXT("selection");
        if(!this->flags.is_set(Flag::SELECTION_UPDATED))
            return true;
        const auto vbo = this->selection_vbo;
        const auto ebo = this->selection_ebo;
        const auto &v = this->selections;
        const auto n = v.size();
        if(!n) {
            this->graphics->set_buffer_size(ebo, 0);
            return true;
        }
        return update_with_state<::update_selections, update_indices<6>>(
            this->graphics, vbo, ebo, 0, 0,
            n, 4 * sizeof(Vertex), n, 6 * sizeof(std::uint32_t),
            rptr(UpdateSelectionData{
                rptr(cbegin(this->selections)),
                std::span{std::as_const(this->sprites)}}));
    };
    const auto update_aabbs = [this] {
        NNGN_LOG_CONTEXT("aabb");
        const auto vbo = this->aabb_vbo;
        const auto ebo = this->aabb_ebo;
        if(!this->m_debug.is_set(Debug::BB)) {
            this->graphics->set_buffer_size(ebo, 0);
            return true;
        }
        const auto &v = this->colliders->aabb();
        if(v.empty()) {
            this->graphics->set_buffer_size(ebo, 0);
            return true;
        }
        const auto &collisions = this->colliders->collisions();
        std::unordered_set<const Collider*> lookup;
        for(auto x : collisions) {
            lookup.insert(x.entity0->collider);
            lookup.insert(x.entity1->collider);
        }
        const auto n = v.size();
        return update_with_state<::update_aabbs, update_indices<6>>(
            this->graphics, vbo, ebo, 0, 0,
            n, 4 * sizeof(Vertex), n, 6 * sizeof(std::uint32_t),
            rptr(UpdateAABBData{v.data(), std::move(lookup)}));
    };
    const auto update_aabb_circles = [this] {
        NNGN_LOG_CONTEXT("aabb circle");
        const auto vbo = this->aabb_circle_vbo;
        const auto ebo = this->aabb_circle_ebo;
        const auto v = std::span{
            const_cast<std::vector<AABBCollider>&>(this->colliders->aabb())};
        if(!this->m_debug.is_set(Debug::CIRCLE) || v.empty()) {
            this->graphics->set_buffer_size(ebo, 0);
            return true;
        }
        return update_span<::update_aabb_circles, update_indices<6>>(
            this->graphics, v, vbo, ebo, 4, 6);
    };
    const auto update_bbs = [this] {
        NNGN_LOG_CONTEXT("bb");
        const auto vbo = this->bb_vbo;
        const auto ebo = this->bb_ebo;
        if(!this->m_debug.is_set(Debug::BB)) {
            this->graphics->set_buffer_size(ebo, 0);
            return true;
        }
        const auto &v = this->colliders->bb();
        if(v.empty()) {
            this->graphics->set_buffer_size(ebo, 0);
            return true;
        }
        const auto &collisions = this->colliders->collisions();
        std::unordered_set<const Collider*> lookup;
        for(auto x : collisions) {
            lookup.insert(x.entity0->collider);
            lookup.insert(x.entity1->collider);
        }
        const auto n = v.size();
        return update_with_state<::update_bbs, update_indices<6>>(
            this->graphics, vbo, ebo, 0, 0,
            n, 4 * sizeof(Vertex), n, 6 * sizeof(std::uint32_t),
            rptr(UpdateBBData{v.data(), std::move(lookup)}));
    };
    const auto update_bb_circles = [this] {
        NNGN_LOG_CONTEXT("bb circle");
        const auto vbo = this->bb_circle_vbo;
        const auto ebo = this->bb_circle_ebo;
        const auto v = std::span{
            const_cast<std::vector<BBCollider>&>(this->colliders->bb())};
        if(!this->m_debug.is_set(Debug::CIRCLE) || v.empty()) {
            this->graphics->set_buffer_size(ebo, 0);
            return true;
        }
        return update_span<::update_aabb_circles, update_indices<6>>(
            this->graphics, v, vbo, ebo, 4, 6);
    };
    const auto update_spheres = [this] {
        NNGN_LOG_CONTEXT("sphere");
        const auto vbo = this->sphere_coll_vbo;
        const auto ebo = this->sphere_coll_ebo;
        const auto v = std::span{
            const_cast<std::vector<SphereCollider>&>(
                this->colliders->sphere())};
        if(!this->m_debug.is_set(Debug::CIRCLE) || v.empty()) {
            this->graphics->set_buffer_size(ebo, 0);
            return true;
        }
        return update_span<::update_spheres, update_indices<6>>(
            this->graphics, v, vbo, ebo, 4, 6);
    };
    const auto update_lights = [this] {
        NNGN_LOG_CONTEXT("lights");
        const auto vbo = this->lights_vbo;
        const auto ebo = this->lights_ebo;
        if(!this->m_debug.is_set(Debug::LIGHT)) {
            this->graphics->set_buffer_size(ebo, 0);
            return true;
        }
        const auto dir = this->lighting->dir_lights();
        const auto point = this->lighting->point_lights();
        const auto n = dir.size() + point.size();
        if(!n) {
            this->graphics->set_buffer_size(ebo, 0);
            return true;
        }
        std::uint64_t voff = {}, eoff = {};
        std::uint64_t ei = {};
        const bool ok = ::update_lights<update_dir_lights>(
                this->graphics, vbo, ebo, &voff, &eoff, &ei, dir,
                this->lighting->shadow_map_far())
            && ::update_lights<update_point_lights>(
                this->graphics, vbo, ebo, &voff, &eoff, &ei, point, {});
        if(!ok)
            return false;
        this->graphics->set_buffer_size(vbo, voff);
        this->graphics->set_buffer_size(ebo, eoff);
        return true;
    };
    const auto update_ranges = [this] {
        NNGN_LOG_CONTEXT("range");
        const auto vbo = this->range_vbo;
        const auto ebo = this->range_ebo;
        if(!this->m_debug.is_set(Debug::LIGHT)) {
            this->graphics->set_buffer_size(ebo, 0);
            return true;
        }
        const auto cv = this->lighting->point_lights();
        const auto v = std::span{const_cast<Light*>(cv.data()), cv.size()};
        if(v.empty()) {
            this->graphics->set_buffer_size(ebo, 0);
            return true;
        }
        return update_span<::update_ranges, update_indices<6 * 6>>(
            this->graphics, v, vbo, ebo, 6 * 4, 6 * 6);
    };
    const auto update_shadow_maps = [this] {
        NNGN_LOG_CONTEXT("shadow maps");
        const auto vbo = this->depth_vbo;
        const auto ebo = this->depth_ebo;
        if(!this->m_debug.is_set(Debug::LIGHT)) {
            this->graphics->set_buffer_size(ebo, 0);
            return true;
        }
        const auto n = static_cast<unsigned>(
            this->lighting->dir_lights().size());
        if(!n) {
            this->graphics->set_buffer_size(ebo, 0);
            return true;
        }
        return ::update_shadow_maps(this->graphics, vbo, ebo, {}, {n, 1});
    };
    const auto update_shadow_cubes = [this] {
        NNGN_LOG_CONTEXT("shadow cubes");
        const auto vbo = this->depth_cube_vbo;
        const auto ebo = this->depth_cube_ebo;
        if(!this->m_debug.is_set(Debug::LIGHT)) {
            this->graphics->set_buffer_size(ebo, 0);
            return true;
        }
        const auto n = static_cast<unsigned>(
            this->lighting->point_lights().size());
        if(!n) {
            this->graphics->set_buffer_size(ebo, 0);
            return true;
        }
        return ::update_shadow_maps(this->graphics, vbo, ebo, {0, 1}, {6, n});
    };
    const auto updated = [&flags = this->flags](auto f, const auto &v) {
        return flags.is_set(f)
            ? (flags.clear(f), true)
            // TODO flag hierarchy
            : std::ranges::any_of(begin(v), end(v), &Renderer::updated);
    };
    NNGN_PROFILE_CONTEXT(renderers);
    const auto sprites_updated = updated(Flag::SPRITES_UPDATED, this->sprites);
    if(sprites_updated && !update_sprites())
        return false;
    const auto translucent_updated =
        updated(Flag::TRANSLUCENT_UPDATED, this->translucent);
    if(translucent_updated && !update_translucent())
        return false;
    const auto cubes_updated = updated(Flag::CUBES_UPDATED, this->cubes);
    if(cubes_updated && !update_cubes())
        return false;
    const auto voxels_updated = updated(Flag::VOXELS_UPDATED, this->voxels);
    if(voxels_updated && !update_voxels())
        return false;
    const auto rect_enabled = this->m_debug & Debug::RECT;
    const auto rect_updated = this->flags.check_and_clear(Flag::RECT_UPDATED)
        || sprites_updated || cubes_updated || voxels_updated;
    if(rect_updated) {
        if(rect_enabled) {
            if(!(update_rect() && update_cube_dbg() && update_voxel_dbg()))
                return false;
        } else {
            this->graphics->set_buffer_size(this->box_ebo, 0);
            this->graphics->set_buffer_size(this->cube_debug_ebo, 0);
            this->graphics->set_buffer_size(this->voxel_debug_ebo, 0);
        }
    }
    return update_text() && update_textbox() && update_selections()
        && update_aabbs() && update_aabb_circles()
        && update_bbs() && update_bb_circles()
        && update_spheres()
        && update_lights() && update_ranges()
        && update_shadow_maps() && update_shadow_cubes();
}

}
