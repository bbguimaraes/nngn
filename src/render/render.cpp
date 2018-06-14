#include <algorithm>
#include <cstring>
#include <span>

#include <sol/table.hpp>

#include "entity.h"

#include "graphics/texture.h"
#include "timing/profile.h"
#include "utils/log.h"
#include "utils/vector.h"

#include "render.h"

using nngn::u32, nngn::u64;

namespace {

template<std::size_t per_obj>
void update_indices(void*, void *p, u64 i, u64 n) {
    nngn::Renderers::gen_quad_idxs(
        i * per_obj / 6, n * per_obj / 6, static_cast<u32*>(p));
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

void update_sprites(nngn::Vertex **p, nngn::SpriteRenderer *x) {
    x->flags.clear(nngn::Renderer::Flag::UPDATED);
    const auto pos = x->pos.xy();
    const auto s = x->size / 2.0f;
    nngn::Renderers::gen_quad_verts(
        p, pos - s, pos + s, -pos.y - x->z_off,
        x->tex, x->uv0, x->uv1);
}

void update_boxes(nngn::Vertex **p, nngn::SpriteRenderer *x) {
    const auto pos = x->pos.xy();
    auto s = x->size / 2.0f;
    nngn::vec2 bl = pos - s, tr = pos + s;
    nngn::Renderers::gen_quad_verts(p, bl, tr, 0, {1, 1, 1});
    s = {0.5f, 0.5f};
    bl = pos - s;
    tr = pos + s;
    nngn::Renderers::gen_quad_verts(p, bl, tr, 0, {1, 0, 0});
    bl.y = x->pos.y + x->z_off - s.y;
    tr.y = x->pos.y + x->z_off + s.y;
    nngn::Renderers::gen_quad_verts(p, bl, tr, 0, {1, 0, 0});
}

}

namespace nngn {

void Renderers::init(Textures *t) {
    this->textures = t;
}

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

void Renderers::set_debug(std::underlying_type_t<Debug> d) {
    auto old = this->m_debug;
    this->m_debug = {d};
    if((old ^ this->m_debug) & Debug::RECT)
        this->flags |= Flag::RECT_UPDATED;
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
        triangle_pipeline = {}, sprite_pipeline = {}, box_pipeline = {},
        triangle_vbo = {}, triangle_ebo = {};
    return (triangle_pipeline = g->create_pipeline({
            .name = "triangle_pipeline",
            .type = Pipeline::Type::TRIANGLE,
            .flags = Pipeline::Flag::DEPTH_TEST,
        }))
        && (sprite_pipeline = g->create_pipeline({
            .name = "sprite_pipeline",
            .type = Pipeline::Type::SPRITE,
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
                }),
            }, {
                .pipeline = sprite_pipeline,
                .buffers = std::to_array<BufferPair>({
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
}

bool Renderers::update() {
    NNGN_LOG_CONTEXT_CF(Renderers);
    const auto update_sprites = [this] {
        NNGN_LOG_CONTEXT("sprites");
        const auto vbo = this->sprite_vbo;
        const auto ebo = this->sprite_ebo;
        return update_span<::update_sprites, update_indices<6>>(
            this->graphics, std::span{this->sprites}, vbo, ebo, 4, 6);
    };
    const auto update_rect = [this] {
        NNGN_LOG_CONTEXT("rect");
        return update_span<::update_boxes, update_indices<3 * 6>>(
            this->graphics, std::span{this->sprites},
            this->box_vbo, this->box_ebo, 3 * 4, 3 * 6);
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
    const auto rect_enabled = this->m_debug & Debug::RECT;
    const auto rect_updated = this->flags.check_and_clear(Flag::RECT_UPDATED)
        || sprites_updated;
    if(rect_updated) {
        if(rect_enabled) {
            if(!update_rect())
                return false;
        } else
            this->graphics->set_buffer_size(this->box_ebo, 0);
    }
    return true;
}

}
