#include <algorithm>
#include <cstring>
#include <span>

#include "entity.h"

#include "timing/profile.h"
#include "utils/literals.h"
#include "utils/log.h"
#include "utils/ranges.h"

#include "gen.h"
#include "render.h"

using namespace nngn::literals;
using nngn::u32, nngn::u64;

namespace {

template<std::size_t per_obj>
void update_quad_indices(void*, void *p, u64 i, u64 n) {
    nngn::Gen::quad_indices(
        i * per_obj / 6, n * per_obj / 6, static_cast<u32*>(p));
}

template<auto vgen, auto egen, typename T>
bool update_span(
    nngn::Graphics *g, std::span<T> s, u32 vbo, u32 ebo,
    std::size_t n_verts, std::size_t n_indcs
) {
    const auto n = s.size();
    if(!n)
        return g->set_buffer_size(ebo, 0);
    return g->update_buffers(
        vbo, ebo, 0, 0,
        n, n_verts * sizeof(nngn::Vertex),
        n, n_indcs * sizeof(u32), s.data(),
        [](void *d, void *vp, u64 i, u64 nw) {
            auto *p = static_cast<nngn::Vertex*>(vp);
            for(auto &x : std::span{
                static_cast<T*>(d) + i,
                static_cast<std::size_t>(nw)
            })
                vgen(&p, &x);
        }, egen);
}

}

namespace nngn {

std::size_t Renderers::n() const {
    return this->sprites.size();
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

void Renderers::set_debug(Debug d) {
    const auto old = this->m_debug;
    const auto diff = old ^ d;
    this->m_debug = {d};
    if(diff & Debug::DEBUG_RENDERERS)
        this->flags.set(Flag::DEBUG_UPDATED);
}

bool Renderers::set_graphics(Graphics *g) {
    constexpr u32
        TRIANGLE_MAX = 3,
        TRIANGLE_VBO_SIZE = TRIANGLE_MAX * sizeof(Vertex),
        TRIANGLE_EBO_SIZE = TRIANGLE_MAX * sizeof(u32);
    using Pipeline = Graphics::PipelineConfiguration;
    using Stage = Graphics::RenderList::Stage;
    using BufferPair = std::pair<u32, u32>;
    constexpr auto vertex = Graphics::BufferConfiguration::Type::VERTEX;
    constexpr auto index = Graphics::BufferConfiguration::Type::INDEX;
    this->graphics = g;
    u32
        triangle_pipeline = {}, box_pipeline = {},
        triangle_vbo = {}, triangle_ebo = {};
    return (triangle_pipeline = g->create_pipeline({
            .name = "triangle_pipeline",
            .type = Pipeline::Type::TRIANGLE,
            .flags = Pipeline::Flag::DEPTH_TEST,
        }))
        && (box_pipeline = g->create_pipeline({
            .name = "box_pipeline",
            .type = Pipeline::Type::TRIANGLE,
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
        && (this->sprite_vbo = g->create_buffer({
            .name = "sprite_vbo",
            .type = vertex,
        }))
        && (this->sprite_ebo = g->create_buffer({
            .name = "sprite_ebo",
            .type = index,
        }))
        && (this->box_vbo = g->create_buffer({
            .name = "box_vbo",
            .type = vertex,
        }))
        && (this->box_ebo = g->create_buffer({
            .name = "box_ebo",
            .type = index,
        }))
        && g->update_buffers(
            triangle_vbo, triangle_ebo, 0, 0,
            1, TRIANGLE_VBO_SIZE, 1, TRIANGLE_EBO_SIZE, nullptr,
            [](void*, void *p, u64, u64) {
                const auto v = std::array<nngn::Vertex, TRIANGLE_MAX>{{
                    {{  0,  16, 0}, {1, 0, 0}},
                    {{-16, -16, 0}, {0, 1, 0}},
                    {{ 16, -16, 0}, {0, 0, 1}}}};
                memcpy(p, std::span{v});
            },
            [](void*, void *p, u64, u64) {
                const auto v = std::array<u32, TRIANGLE_MAX>{0, 1, 2};
                memcpy(p, std::span{v});
            })
        && g->set_render_list(Graphics::RenderList{
            .normal = std::to_array<Stage>({{
                .pipeline = triangle_pipeline,
                .buffers = std::to_array<BufferPair>({
                    {triangle_vbo, triangle_ebo},
                    {this->sprite_vbo, this->sprite_ebo},
                }),
            }}),
            .overlay = std::to_array<Stage>({{
                .pipeline = box_pipeline,
                .buffers = std::to_array<BufferPair>({
                    {this->box_vbo, this->box_ebo},
                }),
            }}),
        });
}

Renderer *Renderers::load(const nngn::lua::table &t) {
    NNGN_LOG_CONTEXT_CF(Renderers);
    const auto load = [&t](auto &v, const char *name) {
        if(v.size() == v.capacity()) {
            Log::l() << "cannot add more " << name << " renderers" << std::endl;
            return static_cast<decltype(v.data())>(nullptr);
        }
        auto x = &v.emplace_back();
        x->load(t);
        x->flags |= Renderer::Flag::UPDATED;
        return x;
    };
    const auto type = chain_cast<Renderer::Type, lua_Integer>(t["type"]);
    switch(type) {
    case Renderer::Type::SPRITE: return load(this->sprites, "sprite");
    case Renderer::Type::N_TYPES:
    default:
        Log::l() << "invalid type: " << static_cast<int>(type) << '\n';
        return nullptr;
    }
}

void Renderers::remove(Renderer *p) {
    const auto remove = [this, p]<typename T>(std::vector<T> *v, auto flag) {
        auto *const dp = static_cast<T*>(p);
        const_time_erase(v, dp);
        if(p != &*v->end()) {
            p->entity->renderer = p;
            p->flags |= Renderer::Flag::UPDATED;
        }
        this->flags |= flag;
    };
    if(contains(this->sprites, *p))
        remove(&this->sprites, Flag::SPRITES_UPDATED);
}

bool Renderers::update(void) {
    NNGN_LOG_CONTEXT_CF(Renderers);
    const auto updated = [&flags = this->flags](auto f, const auto &v) {
        return flags.is_set(f)
            ? (flags.clear(f), true)
            // TODO flag hierarchy
            : std::ranges::any_of(begin(v), end(v), &Renderer::updated);
    };
    const auto sprites_updated = updated(Flag::SPRITES_UPDATED, this->sprites);
    return this->update_renderers(sprites_updated)
        && this->update_debug(sprites_updated);
}

bool Renderers::update_renderers(bool sprites_updated) {
    NNGN_PROFILE_CONTEXT(renderers);
    const auto update_sprites = [this] {
        NNGN_LOG_CONTEXT("sprites");
        const auto vbo = this->sprite_vbo;
        const auto ebo = this->sprite_ebo;
        return update_span<Gen::sprite, update_quad_indices<6>>(
            this->graphics, std::span{this->sprites}, vbo, ebo, 4, 6);
    };
    return !sprites_updated || update_sprites();
}

bool Renderers::update_debug(bool sprites_updated) {
    NNGN_PROFILE_CONTEXT(renderers_debug);
    const auto update_sprite_debug = [this] {
        NNGN_LOG_CONTEXT("sprite_debug");
        return update_span<Gen::sprite_debug, update_quad_indices<3 * 6>>(
            this->graphics, std::span{this->sprites},
            this->box_vbo, this->box_ebo, 3_z * 4_z, 3_z * 6_z);
    };
    const auto update = [
        this,
        enabled = this->m_debug.is_set(Debug::DEBUG_RENDERERS),
        updated = this->flags.check_and_clear(Flag::DEBUG_UPDATED)
    ](bool flag, auto f, auto ebo) {
        return !(updated || flag)
            || (enabled ? f() : this->graphics->set_buffer_size(ebo, 0));
    };
    return update(sprites_updated, update_sprite_debug, this->box_ebo);
}

}
