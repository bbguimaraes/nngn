#include <vector>

#include "graphics/graphics.h"

#include "grid.h"

namespace nngn {

bool Grid::set_graphics(Graphics *g) {
    static constexpr auto max = 1024;
    this->graphics = g;
    return (this->m_vbo = g->create_buffer({
            .name = "grid_vbo",
            .type = Graphics::BufferConfiguration::Type::VERTEX,
            .size = 8 * max * sizeof(Vertex),
        }))
        && (this->m_ebo = g->create_buffer({
            .name = "grid_ebo",
            .type = Graphics::BufferConfiguration::Type::INDEX,
            .size = 8 * max * sizeof(u32),
        }));
}

bool Grid::set_enabled(bool e) {
    this->m_enabled = e;
    return this->update();
}

bool Grid::update() const {
    constexpr std::uint32_t n_dim = 2, lines_per_dim = 2, vertices_per_line = 2;
    constexpr auto n = n_dim * lines_per_dim * vertices_per_line;
    if(!(this->m_enabled && this->size)) {
       this->graphics->set_buffer_size(m_ebo, 0);
       return true;
    }
    const bool ok = this->graphics->write_to_buffer(
            this->m_vbo, 0, this->size, n * sizeof(Vertex),
            const_cast<Grid*>(this),
            [](void *data, void *vp, std::uint64_t i, std::uint64_t nw) {
                constexpr vec3 norm = {0, 0, 1};
                auto *g = static_cast<const Grid*>(data);
                auto *p = static_cast<Vertex*>(vp);
                const auto max = static_cast<float>(g->size) * g->spacing;
                for(const auto e = i + nw; i < e; ++i) {
                    const auto fi = static_cast<float>(i);
                    *(p++) = {{-max, fi * -g->spacing, 0}, norm, g->color};
                    *(p++) = {{ max, fi * -g->spacing, 0}, norm, g->color};
                    *(p++) = {{-max, fi *  g->spacing, 0}, norm, g->color};
                    *(p++) = {{ max, fi *  g->spacing, 0}, norm, g->color};
                    *(p++) = {{fi * -g->spacing, -max, 0}, norm, g->color};
                    *(p++) = {{fi * -g->spacing,  max, 0}, norm, g->color};
                    *(p++) = {{fi *  g->spacing, -max, 0}, norm, g->color};
                    *(p++) = {{fi *  g->spacing,  max, 0}, norm, g->color};
                }
            })
        && this->graphics->write_to_buffer(
            this->m_ebo, 0, n * this->size, sizeof(std::uint32_t), {},
            [](void*, void *vp, std::uint64_t i, std::uint64_t nw) {
                auto *p = static_cast<std::uint32_t*>(vp);
                for(const auto e = i + nw; i < e; ++i)
                    *p++ = static_cast<std::uint32_t>(i);
            });
    if(!ok)
        return false;
    this->graphics->set_buffer_size(
        this->m_vbo, this->size * n * sizeof(Vertex));
    this->graphics->set_buffer_size(
        this->m_ebo, this->size * n * sizeof(u32));
    return true;
}

bool Grid::set_dimensions(float sp, unsigned sz) {
    this->spacing = sp;
    this->size = sz;
    return this->m_enabled ? this->update() : true;
}

bool Grid::set_color(float v0, float v1, float v2) {
    this->color = {v0, v1, v2};
    return this->m_enabled ? this->update() : true;
}

}
