#ifndef NNGN_GRID_H
#define NNGN_GRID_H

#include "math/vec3.h"
#include "utils/def.h"

namespace nngn {

struct Graphics;

class Grid {
    Graphics *graphics = nullptr;
    std::size_t m_size = 0;
    vec3 color = {1, 1, 1};
    float m_spacing = 0;
    u32 m_vbo = {}, m_ebo = {};
    bool m_enabled = false;
    bool update() const;
public:
    bool enabled() const { return this->m_enabled; }
    std::size_t size(void) const { return this->m_size; }
    float spacing(void) const { return this->m_spacing; }
    u32 vbo() const { return this->m_vbo; }
    u32 ebo() const { return this->m_ebo; }
    bool set_graphics(Graphics *g);
    bool set_enabled(bool e);
    bool set_dimensions(float spacing, unsigned size);
    bool set_color(float v0, float v1, float v2);
};

}

#endif
