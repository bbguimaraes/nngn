#include "lua/state.h"

#include "entity.h"

#include "light.h"

using nngn::Light;
using nngn::Lighting;

namespace {

template<void (Light::*f)(const nngn::vec3&)>
static void s(Light *l, float v0, float v1, float v2) {
    (l->*f)({v0, v1, v2});
}

template<void (Light::*f)(const nngn::vec4&)>
static void s(Light *l, float v0, float v1, float v2, float v3) {
    (l->*f)({v0, v1, v2, v3});
}

template<const nngn::vec4 &(Lighting::*f)() const>
static std::tuple<float, float, float, float> g(const Lighting &l) {
    auto r = (l.*f)(); return {r[0], r[1], r[2], r[3]};
}

template<void (Lighting::*f)(const nngn::vec3&)>
static void s(Lighting *l, float v0, float v1, float v2) {
    (l->*f)({v0, v1, v2});
}

template<void (Lighting::*f)(const nngn::vec4&)>
static void s(Lighting *l, float v0, float v1, float v2, float v3) {
    (l->*f)({v0, v1, v2, v3});
}

void set_ambient_anim(Lighting *l, const nngn::lua::table &t) {
    nngn::LightAnimation a;
    a.load(t);
    l->set_ambient_anim(a);
}

}

NNGN_LUA_PROXY(Light,
    "POINT", nngn::lua::var(Light::Type::POINT),
    "DIR", nngn::lua::var(Light::Type::DIR),
    "entity", [](const Light &l) { return l.e; },
    "pos", [](const Light &l) { return l.pos.as_tuple(); },
    "dir", [](const Light &l) { return l.dir.as_tuple(); },
    "color", [](const Light &l) { return l.color.as_tuple(); },
    "att", [](const Light &l) { return l.att.as_tuple(); },
    "spec", [](const Light &l) { return l.spec; },
    "cutoff", [](const Light &l) { return l.cutoff; },
    "set_dir", s<&Light::set_dir>,
    "set_color", s<&Light::set_color>,
    "set_att", [](Light &l, nngn::lua::state_arg lua) { l.set_att(lua); },
    "set_spec", &Light::set_spec,
    "set_cutoff", &Light::set_cutoff)
NNGN_LUA_PROXY(Lighting,
    "enabled", &Lighting::enabled,
    "ambient_light", g<&Lighting::ambient_light>,
    "n_dir_lights", [](const Lighting &l) { return l.dir_lights().size(); },
    "n_point_lights", [](const Lighting &l) { return l.point_lights().size(); },
    "dir_light", [](const Lighting &l, size_t i)
        { return &l.dir_lights()[i]; },
    "point_light", [](const Lighting &l, size_t i)
        { return &l.point_lights()[i]; },
    "set_enabled", &Lighting::set_enabled,
    "set_ambient_light", s<&Lighting::set_ambient_light>,
    "set_ambient_anim", set_ambient_anim,
    "add_light", &Lighting::add_light,
    "remove_light", &Lighting::remove_light)
