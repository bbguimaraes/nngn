#ifndef NNGN_MAP_H
#define NNGN_MAP_H

#include <vector>

#include "lua/table.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include "utils/def.h"
#include "utils/flags.h"

namespace nngn {

struct Graphics;
class Textures;

class Map {
public:
    static std::vector<uvec2> load_tiles(
        std::size_t width, std::size_t height, nngn::lua::table_view tiles);
    void init(Textures *t) { this->textures = t; }
    u32 vbo() const { return this->m_vbo; }
    u32 ebo() const { return this->m_ebo; }
    bool set_max(std::size_t n);
    bool set_graphics(Graphics *g);
    bool load(
        unsigned int tex, float sprite_scale,
        float trans_x, float trans_y, float scale_x, float scale_y,
        unsigned int width, unsigned int height,
        nngn::lua::table_view tiles);
    bool enabled() const { return this->m_flags.is_set(Flag::ENABLED); }
    bool perspective() const
        { return this->m_flags.is_set(Flag::PERSPECTIVE); }
    bool set_enabled(bool e);
private:
    enum Flag : uint8_t { ENABLED = 1u << 0, PERSPECTIVE = 1u << 1 };
    Textures *textures = nullptr;
    Graphics *graphics = nullptr;
    Flags<Flag> m_flags = {Flag::ENABLED | Flag::PERSPECTIVE};
    unsigned int tex = 0;
    uvec2 size = {};
    float sprite_scale = 1.0f;
    vec2 trans = {}, scale = {};
    std::vector<uvec2> uv = {};
    std::size_t max = {};
    u32 m_vbo = {}, m_ebo = {};
    bool gen() const;
};

}

#endif
