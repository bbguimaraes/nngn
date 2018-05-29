#include "lua/function.h"
#include "lua/register.h"
#include "lua/table.h"

#include "entity.h"

#include "light.h"

using nngn::Light;
using nngn::Lighting;

NNGN_LUA_DECLARE_USER_TYPE(Entity)
NNGN_LUA_DECLARE_USER_TYPE(nngn::Sun, "Sun")

namespace {

auto number(float v) {
    return nngn::narrow<lua_Number>(v);
}

auto vec3(nngn::vec3 v) {
    return std::tuple{number(v[0]), number(v[1]), number(v[2])};
}

auto vec4(nngn::vec4 v) {
    return std::tuple{number(v[0]), number(v[1]), number(v[2]), number(v[3])};
}

/* XXX mingw
template<float Light::*p>
auto get(const Light &l) {
    return number(l.*p);
}

template<nngn::vec3 Light::*p>
auto get(const Light &l) {
    return vec3(l.*p);
}

template<nngn::vec4 Light::*p>
auto get(const Light &l) {
    return vec4(l.*p);
}*/

template<void (Light::*f)(const nngn::vec3&)>
void set(Light *l, float v0, float v1, float v2) {
    (l->*f)({v0, v1, v2});
}

template<void (Light::*f)(const nngn::vec4&)>
void set(Light *l, float v0, float v1, float v2, float v3) {
    (l->*f)({v0, v1, v2, v3});
}

template<const nngn::vec4 &(Lighting::*f)() const>
auto get(const Lighting &l) {
    auto ret = (l.*f)();
    return std::tuple{
        nngn::narrow<lua_Number>(ret[0]), nngn::narrow<lua_Number>(ret[1]),
        nngn::narrow<lua_Number>(ret[2]), nngn::narrow<lua_Number>(ret[3]),
    };
}

template<void (Lighting::*f)(const nngn::vec3&)>
void set(Lighting *l, float v0, float v1, float v2) {
    (l->*f)({v0, v1, v2});
}

template<void (Lighting::*f)(const nngn::vec4&)>
void set(Lighting *l, float v0, float v1, float v2, float v3) {
    (l->*f)({v0, v1, v2, v3});
}

auto n_dir_lights(const Lighting &l) {
    return nngn::narrow<lua_Integer>(l.dir_lights().size());
}

auto n_point_lights(const Lighting &l) {
    return nngn::narrow<lua_Integer>(l.point_lights().size());
}

auto dir_light(const Lighting &l, lua_Integer i) {
    return &l.dir_lights()[nngn::narrow<std::size_t>(i)];
}

auto point_light(const Lighting &l, lua_Integer i) {
    return &l.point_lights()[nngn::narrow<std::size_t>(i)];
}

void set_ambient_anim(Lighting *l, nngn::lua::table_view t) {
    nngn::LightAnimation a = {};
    a.load(t);
    l->set_ambient_anim(a);
}

void register_light(nngn::lua::table_view t) {
    t["POINT"] = Light::Type::POINT;
    t["DIR"] = Light::Type::DIR;
    t["entity"] = nngn::lua::value_accessor<&Light::e>;
    t["pos"] = [](const Light &c) { return vec3(c.pos); };
    t["dir"] = [](const Light &c) { return vec3(c.dir); };
    t["color"] = [](const Light &c) { return vec4(c.color); };
    t["att"] = [](const Light &c) { return vec3(c.att); };
    t["spec"] = [](const Light &c) { return number(c.spec); };
    t["cutoff"] = [](const Light &c) { return number(c.cutoff); };
    t["set_dir"] = set<&Light::set_dir>;
    t["set_color"] = set<&Light::set_color>;
    t["set_att"] = [](Light &l, nngn::lua::state_view lua) { l.set_att(lua); };
    t["set_spec"] = &Light::set_spec;
    t["set_cutoff"] = &Light::set_cutoff;
}

void register_lighting(nngn::lua::table_view t) {
    t["enabled"] = &Lighting::enabled;
    t["update_sun"] = &Lighting::update_sun;
    t["ambient_light"] = get<&Lighting::ambient_light>;
    t["n_dir_lights"] = n_dir_lights;
    t["n_point_lights"] = n_point_lights;
    t["dir_light"] = dir_light;
    t["point_light"] = point_light;
    t["sun_light"] = [](const Lighting &l) { return l.sun_light(); };
    t["sun"] = &Lighting::sun;
    t["set_enabled"] = &Lighting::set_enabled;
    t["set_update_sun"] = &Lighting::set_update_sun;
    t["set_ambient_light"] = set<&Lighting::set_ambient_light>;
    t["set_ambient_anim"] = set_ambient_anim;
    t["set_sun_light"] = &Lighting::set_sun_light;
    t["add_light"] = &Lighting::add_light;
    t["remove_light"] = &Lighting::remove_light;
}

}

NNGN_LUA_DECLARE_USER_TYPE(Light)
NNGN_LUA_DECLARE_USER_TYPE(Lighting)
NNGN_LUA_PROXY(Light, register_light)
NNGN_LUA_PROXY(Lighting, register_lighting)
