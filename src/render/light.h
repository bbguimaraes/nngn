#ifndef NNGN_LIGHT_H
#define NNGN_LIGHT_H

#include <chrono>
#include <vector>

#include "const.h"

#include "graphics/graphics.h"
#include "math/mat4.h"
#include "math/vec3.h"
#include "utils/flags.h"

#include "animation.h"
#include "sun.h"

struct lua_State;
struct Entity;

namespace nngn {

struct Timing;

struct Light {
    enum Type : uint8_t { POINT, DIR };
    Type type = POINT;
    Entity *e = nullptr;
    vec3 pos = {}, dir = {};
    vec4 color = {};
    vec3 att = {1, 0, 0};
    float spec = 1.0f, cutoff = 0.0f;
    bool updated = true;
    static constexpr vec3 att_for_range(float r)
        { return {1, 4.5f / r, 75.0f / (r * r)}; }
    constexpr Light() = default;
    explicit constexpr Light(Type t) noexcept : type(t) {}
    void set_pos(const vec3 &p);
    void set_dir(const vec3 &p);
    void set_color(const vec4 &c);
    void set_att(lua_State *L);
    void set_spec(float v);
    void set_cutoff(float v);
    float range() const { return 4.5f / this->att[1]; }
    vec3 ortho_view_pos() const;
    void write_to_ubo_dir(LightsUBO *ubo, size_t i) const;
    void write_to_ubo_point(LightsUBO *ubo, size_t i, bool zsprites) const;
};

class Lighting {
public:
    static constexpr size_t MAX_LIGHTS = NNGN_MAX_LIGHTS;
private:
    enum Flag : uint8_t {
        ENABLED = 1u << 0,
        ZSPRITES = 1u << 1,
        UPDATE_SUN = 1u << 2,
        UPDATED = 1u << 3,
        VIEW_UPDATED = 1u << 4,
    };
    Math *math = nullptr;
    Flags<Flag> flags = {Flag::ENABLED | Flag::UPDATE_SUN | Flag::UPDATED};
    vec3 view_pos = {};
    vec4 m_ambient_light = vec4(1);
    LightAnimation m_ambient_anim = {};
    size_t n_dir = 0, n_point = 0;
    std::array<Light, MAX_LIGHTS> m_dir_lights = {}, m_point_lights = {};
    Light *m_sun_light = nullptr;
    LightsUBO m_ubo = {};
    Sun m_sun = {};
public:
    void init(Math *m) { this->math = m; }
    bool enabled() const { return this->flags.is_set(Flag::ENABLED); }
    bool zsprites(void) const { return this->flags.is_set(Flag::ZSPRITES); }
    bool update_sun(void) const { return this->flags.is_set(Flag::UPDATE_SUN); }
    const vec4 &ambient_light() const { return this->m_ambient_light; }
    std::span<const Light> dir_lights() const;
    std::span<Light> dir_lights();
    std::span<const Light> point_lights() const;
    std::span<Light> point_lights();
    Light *sun_light() { return this->m_sun_light; }
    const Light *sun_light() const { return this->m_sun_light; }
    const LightsUBO &ubo() const { return this->m_ubo; }
    Sun *sun() { return &this->m_sun; }
    void set_enabled(bool b);
    void set_zsprites(bool b);
    void set_update_sun(bool b);
    void set_ambient_light(const vec4 &v);
    void set_ambient_anim(LightAnimation a);
    void set_sun_light(Light *l) { this->m_sun_light = l; }
    Light *add_light(Light::Type t);
    void remove_light(Light *l);
    bool update(const Timing &t);
    void update_view(const vec3 &eye);
};

inline std::span<const Light> Lighting::dir_lights() const
    { return std::span{this->m_dir_lights}.subspan(0, this->n_dir); }
inline std::span<Light> Lighting::dir_lights()
    { return std::span{this->m_dir_lights}.subspan(0, this->n_dir); }
inline std::span<const Light> Lighting::point_lights() const
    { return std::span{this->m_point_lights}.subspan(0, this->n_point); }
inline std::span<Light> Lighting::point_lights()
    { return std::span{this->m_point_lights}.subspan(0, this->n_point); }

}

#endif
