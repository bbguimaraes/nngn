#include <algorithm>
#include <cstring>
#include <span>

#include <sol/table.hpp>

#include "entity.h"

#include "font/font.h"
#include "font/text.h"
#include "font/textbox.h"
#include "graphics/texture.h"
#include "timing/profile.h"
#include "utils/log.h"
#include "utils/vector.h"

#include "grid.h"
#include "render.h"

using nngn::u8, nngn::u32, nngn::u64;

namespace {

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
        x->tex, x->uv0, x->uv1);
}

void update_sprites_persp(nngn::Vertex **p, nngn::SpriteRenderer *x) {
    x->flags.clear(nngn::Renderer::Flag::UPDATED);
    const auto s = x->size / 2.0f;
    const nngn::vec2 bl = {x->pos.x - s.x, -s.y - x->z_off};
    nngn::Renderers::gen_quad_verts_persp(
        p, bl, {x->pos.x + s.x, bl.y + x->size.y},
        x->pos.y + x->z_off, x->tex, x->uv0, x->uv1);
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
            &p, cpos, cpos + size, color, static_cast<u32>(c),
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
    auto *p = static_cast<nngn::Vertex*>(vp);
    const auto *const t = static_cast<const nngn::Textbox*>(data);
    nngn::Renderers::gen_quad_verts(&p, t->title_bl, t->title_tr, 0, {0, 0, 1});
    nngn::Renderers::gen_quad_verts(&p, t->str_bl, t->str_tr, 0, {1, 1, 1});
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
            0, {1, 1, 0});
    }
    *it_p = it;
}

}

namespace nngn {

void Renderers::init(
    Textures *t, const Fonts *f, const Textbox *tb, const Grid *g
) {
    this->textures = t;
    this->fonts = f;
    this->textbox = tb;
    this->grid = g;
}

std::size_t Renderers::n() const {
    return this->sprites.size()
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

void Renderers::set_debug(std::underlying_type_t<Debug> d) {
    auto old = this->m_debug;
    this->m_debug = {d};
    if((old ^ this->m_debug) & Debug::RECT)
        this->flags |= Flag::RECT_UPDATED;
}

void Renderers::set_perspective(bool p) {
    constexpr auto f =
        Flag::SPRITES_UPDATED
        | Flag::CUBES_UPDATED;
    this->flags.set(Flag::PERSPECTIVE, p);
    this->flags.set(f);
}

bool Renderers::set_graphics(Graphics *g) {
    constexpr u32
        TRIANGLE_MAX = 3,
        TRIANGLE_VBO_SIZE = TRIANGLE_MAX * sizeof(Vertex),
        TRIANGLE_EBO_SIZE = TRIANGLE_MAX * sizeof(u32),
        TEXTBOX_MAX = 1,
        SELECTION_MAX = 1024;
    using Pipeline = Graphics::PipelineConfiguration;
    using Stage = Graphics::RenderList::Stage;
    using BufferPair = std::pair<u32, u32>;
    constexpr auto vertex = Graphics::BufferConfiguration::Type::VERTEX;
    constexpr auto index = Graphics::BufferConfiguration::Type::INDEX;
    this->graphics = g;
    u32
        triangle_pipeline = {}, sprite_pipeline = {}, voxel_pipeline = {},
        box_pipeline = {}, font_pipeline = {}, line_pipeline = {},
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
        && (voxel_pipeline = g->create_pipeline({
            .name = "voxel_pipeline",
            .type = Pipeline::Type::VOXEL,
            .flags = static_cast<Pipeline::Flag>(
                Pipeline::Flag::DEPTH_TEST
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
                .pipeline = voxel_pipeline,
                .buffers = std::to_array<BufferPair>({
                    {this->voxel_vbo, this->voxel_ebo},
                }),
            }, {
                .pipeline = triangle_pipeline,
                .buffers = std::to_array<BufferPair>({
                    {triangle_vbo, triangle_ebo},
                    {this->cube_vbo, this->cube_ebo},
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
                    {this->cube_debug_vbo, this->cube_debug_ebo},
                    {this->voxel_debug_vbo, this->voxel_debug_ebo},
                    {this->selection_vbo, this->selection_ebo},
                }),
            }, {
                .pipeline = line_pipeline,
                .buffers = std::to_array<BufferPair>({
                    {this->grid->vbo(), this->grid->ebo()},
                }),
            }}),
            .hud = std::to_array<Stage>({{
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
        return this->flags.is_set(Flag::PERSPECTIVE)
            ? update_span<::update_sprites_persp, update_indices<6>>(
                this->graphics, std::span{this->sprites}, vbo, ebo, 4, 6)
            : update_span<::update_sprites_ortho, update_indices<6>>(
                this->graphics, std::span{this->sprites}, vbo, ebo, 4, 6);
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
    return update_text() && update_textbox() && update_selections();
}

}
