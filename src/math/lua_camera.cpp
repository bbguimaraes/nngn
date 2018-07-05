#include "lua/state.h"
#include "timing/timing.h"

#include "camera.h"

using nngn::Camera;

namespace {

template<nngn::vec3 Camera::*p>
std::tuple<float, float, float> g(const Camera &c) {
    return {(c.*p).x, (c.*p).y, (c.*p).z};
}

template<nngn::vec3 (Camera::*f)() const>
std::tuple<float, float, float> gf(const Camera &c) {
    const auto v = (c.*f)(); return {v.x, v.y, v.z};
}

template<nngn::vec3 Camera::*p>
void s(Camera *s, float v0, float v1, float v2) {
    s->*p = {v0, v1, v2};
}

template<void (Camera::*f)(const nngn::vec3&)>
void sf(Camera *s, float v0, float v1, float v2) {
    (s->*f)({v0, v1, v2});
}

void look_at(
    Camera &c,
    float px, float py, float pz,
    float ex, float ey, float ez,
    float ux, float uy, float uz
) {
    c.look_at({px, py, pz}, {ex, ey, ez}, {ux, uy, uz});
}

}

NNGN_LUA_PROXY(Camera,
    "NEAR", nngn::lua::var(Camera::NEAR),
    "FAR", nngn::lua::var(Camera::FAR),
    "fov", &Camera::fov,
    "pos", gf<&Camera::pos>,
    "vel", g<&Camera::v>,
    "max_vel", [](const Camera &c) { return c.max_v; },
    "max_rot_vel", [](const Camera &c) { return c.max_rv; },
    "max_zoom_vel", [](const Camera &c) { return c.max_zv; },
    "damp", [](const Camera &c) { return c.damp; },
    "acc", g<&Camera::a>,
    "rot", g<&Camera::r>,
    "rot_vel", g<&Camera::rv>,
    "rot_acc", g<&Camera::ra>,
    "zoom", [](const Camera &c) { return c.zoom; },
    "zoom_vel", [](const Camera &c) { return c.zoom_v; },
    "zoom_acc", [](const Camera &c) { return c.zoom_a; },
    "screen", [](const Camera &c)
        { return std::tuple(c.screen.x, c.screen.y); },
    "fov_y", [](const Camera &c) { return c.fov_y; },
    "set_fov_y", &Camera::set_fov_y,
    "dash", [](const Camera &c) { return c.flags.is_set(Camera::Flag::DASH); },
    "eye", gf<&Camera::eye>,
    "up", gf<&Camera::up>,
    "fov_z", &Camera::fov_z,
    "dash", &Camera::dash,
    "perspective", &Camera::perspective,
    "set_screen", [](Camera &c, uint32_t x, uint32_t y)
        { c.set_screen({x, y}); },
    "set_pos", sf<&Camera::set_pos>,
    "set_vel", s<&Camera::v>,
    "set_max_vel", [](Camera &c, float m) { c.max_v = m; },
    "set_max_rot_vel", [](Camera &c, float m) { c.max_rv = m; },
    "set_max_zoom_vel", [](Camera &c, float m) { c.max_zv = m; },
    "set_damp", [](Camera &c, float d) { c.damp = d; },
    "set_acc", s<&Camera::a>,
    "set_rot", sf<&Camera::set_rot>,
    "set_rot_vel", s<&Camera::rv>,
    "set_rot_acc", s<&Camera::ra>,
    "set_zoom", &Camera::set_zoom,
    "set_zoom_vel", [](Camera &c, float v) { c.zoom_v = v; },
    "set_zoom_acc", [](Camera &c, float a) { c.zoom_a = a; },
    "set_dash", &Camera::set_dash,
    "set_perspective", &Camera::set_perspective,
    "look_at", look_at,
    "update", &Camera::update)
