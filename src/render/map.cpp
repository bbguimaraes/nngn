#include "graphics/graphics.h"
#include "graphics/texture.h"
#include "utils/literals.h"
#include "utils/log.h"

#include "gen.h"
#include "map.h"
#include "render.h"

using namespace nngn::literals;

namespace {

auto gen_uv(
    const std::vector<nngn::uvec2> &uv, unsigned width, float sprite_scale,
    std::uint64_t x, std::uint64_t y
) {
    const auto i = static_cast<std::size_t>(width * y + x);
    auto uv0 = static_cast<nngn::vec2>(uv[i]);
    auto uv1 = uv0 + nngn::vec2{1};
    uv0 /= sprite_scale;
    uv1 /= sprite_scale;
    uv0.y = 1 - uv0.y;
    uv1.y = 1 - uv1.y;
    return std::tuple{uv0, uv1};
}

}

namespace nngn {

bool Map::set_graphics(Graphics *g) {
    this->graphics = g;
    this->m_vbo = this->m_ebo = {};
    return this->set_max(this->max);
}

bool Map::set_max(std::size_t n) {
    NNGN_LOG_CONTEXT_CF(Map);
    const auto vsize = 4 * n * sizeof(Vertex);
    const auto esize = 4 * n * sizeof(u32);
    u32 new_vbo = this->m_vbo, new_ebo = this->m_ebo;
    if(!this->m_vbo) {
        new_vbo = this->graphics->create_buffer({
            .name = "map_vbo",
            .type = Graphics::BufferConfiguration::Type::VERTEX,
            .size = vsize,
        });
        new_ebo = this->graphics->create_buffer({
            .name = "map_ebo",
            .type = Graphics::BufferConfiguration::Type::INDEX,
            .size = esize,
        });
    } else if(!(this->graphics->set_buffer_capacity(this->m_vbo, vsize)
            && this->graphics->set_buffer_capacity(this->m_ebo, esize)))
        new_vbo = {};
    if(!(new_vbo && new_ebo))
        return false;
    this->max = n;
    this->m_vbo = new_vbo;
    this->m_ebo = new_ebo;
    return true;
}

std::vector<uvec2> Map::load_tiles(
    size_t width, size_t height, nngn::lua::table_view t
) {
    std::vector<uvec2> ret;
    ret.reserve(width * height);
    for(size_t i = 1; i <= width * height * 2; i += 2)
        ret.emplace_back(t[i], t[i + 1]);
    return ret;
}

bool Map::gen() const {
    if(!this->graphics)
        return true;
    constexpr auto vgen = [](
        void *d, void *vp, std::uint64_t i, std::uint64_t n
    ) {
        auto *p = static_cast<Vertex*>(vp);
        const auto *const m = static_cast<const Map*>(d);
        const auto t = m->trans
            - static_cast<vec2>(m->size - 1u)
            * m->scale / 2.0f;
        auto x = i % m->size.x;
        for(auto y = i / m->size.x;; ++y) {
            const auto fy = static_cast<float>(y);
            for(; x < m->size.x; ++x) {
                const auto fx = static_cast<float>(x);
                const auto [uv0, uv1] =
                    gen_uv(m->uv, m->size.x, m->sprite_scale, x, y);
                Gen::quad_vertices(&p,
                    t + m->scale * vec2(fx - 0.5f, fy - 0.5f),
                    t + m->scale * vec2(fx + 0.5f, fy + 0.5f),
                    0, {0, 0, 1}, m->tex, uv0, uv1);
                if(!--n)
                    return;
            }
            x = 0;
        }
    };
    constexpr auto egen = [](void*, void *p, std::uint64_t i, std::uint64_t n) {
        Gen::quad_indices(i, n, static_cast<std::uint32_t*>(p));
    };
    const auto n = this->size.x * this->size.y;
    if(!n)
        return this->graphics->set_buffer_size(this->m_ebo, 0);
    return this->graphics->write_to_buffer(
            this->m_vbo, 0, n, 4 * sizeof(Vertex), const_cast<Map*>(this), vgen)
        && this->graphics->write_to_buffer(
            this->m_ebo, 0, n, 6 * sizeof(u32), {}, egen)
        && this->graphics->set_buffer_size(
            this->m_vbo, n * 4_z * sizeof(Vertex))
        && this->graphics->set_buffer_size(
            this->m_ebo, n * 6_z * sizeof(u32));
}

bool Map::load(
    uint32_t t, float sscale,
    float trans_x, float trans_y, float scale_x, float scale_y,
    unsigned width, unsigned height, nngn::lua::table_view tiles
) {
    NNGN_LOG_CONTEXT_CF(Map);
    if(this->textures) {
        if(this->tex)
            this->textures->remove(this->tex);
        if(t)
            this->textures->add_ref(t);
    }
    this->uv = Map::load_tiles(width, height, tiles);
    this->tex = t;
    this->sprite_scale = sscale;
    this->trans = {trans_x, trans_y};
    this->scale = {scale_x, scale_y};
    this->size = {width, height};
    return this->gen();
}

bool Map::set_enabled(bool e) {
    return !this->graphics || this->graphics->set_buffer_size(
        this->m_ebo,
        (this->m_flags.set(Flag::ENABLED, e) & Flag::ENABLED)
            ? 6_z * this->size.x * this->size.y * sizeof(uint32_t)
            : 0);
}

}
