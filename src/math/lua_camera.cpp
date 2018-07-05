#include "lua/function.h"
#include "lua/register.h"
#include "lua/table.h"
#include "timing/timing.h"

#include "camera.h"

using nngn::u32;
using nngn::Camera;

NNGN_LUA_DECLARE_USER_TYPE(nngn::Timing, "Timing")

namespace {

auto number(float v) {
    return static_cast<lua_Number>(v);
}

auto vec3(nngn::vec3 v) {
    return std::tuple{number(v.x), number(v.y), number(v.z)};
}

auto matrix(nngn::mat4 v) {
    return std::tuple{
        number(v.m[0]), number(v.m[1]), number(v.m[2]), number(v.m[3]),
        number(v.m[4]), number(v.m[5]), number(v.m[6]), number(v.m[7]),
        number(v.m[8]), number(v.m[9]), number(v.m[10]), number(v.m[11]),
        number(v.m[12]), number(v.m[13]), number(v.m[14]), number(v.m[15]),
    };
}

/* XXX mingw
template<float Camera::*p>
auto get(const Camera &c) {
    return number(c.*p);
}

template<nngn::vec3 Camera::*p>
auto get(const Camera &c) {
    return vec3(c.*p);
}

template<nngn::mat4 Camera::*p>
auto get(const Camera &c) {
    return matrix(c.*p);
}*/

template<float (Camera::*f)(void) const>
auto get(const Camera &c) {
    return number((c.*f)());
}

template<nngn::vec3 (Camera::*f)(void) const>
auto get(const Camera &c) {
    return vec3((c.*f)());
}

template<nngn::vec3 Camera::*p>
void set(Camera *s, float v0, float v1, float v2) {
    s->*p = {v0, v1, v2};
}

template<void (Camera::*f)(nngn::vec3)>
void set_f(Camera *s, float v0, float v1, float v2) {
    (s->*f)({v0, v1, v2});
}

auto fov(lua_Number z, lua_Number w) {
    return number(Camera::fov(nngn::narrow<float>(z), nngn::narrow<float>(w)));
}

auto screen(const Camera &c) {
    return std::tuple{c.screen.x, c.screen.y};
}

bool ignore_limits(const Camera &c) {
    return c.flags.is_set(Camera::Flag::IGNORE_LIMITS);
}

void set_ignore_limits(Camera &c, bool b) {
    c.set_ignore_limits(b);
}

auto world_to_view(const Camera &c, float v0, float v1, float v2) {
    return vec3(c.world_to_view({v0, v1, v2}));
}

auto view_to_clip(const Camera &c, float v0, float v1, float v2) {
    return vec3(c.view_to_clip({v0, v1, v2}));
}

auto clip_to_view(const Camera &c, float v0, float v1, float v2) {
    return vec3(c.view_to_clip({v0, v1, v2}));
}

auto view_to_world(const Camera &c, float v0, float v1, float v2) {
    return vec3(c.view_to_world({v0, v1, v2}));
}

void set_screen(Camera &c, lua_Integer x, lua_Integer y) {
    c.set_screen({nngn::narrow<u32>(x), nngn::narrow<u32>(y)});
}

void set_limits(
    Camera &c,
    float bl_x, float bl_y, float bl_z,
    float tr_x, float tr_y, float tr_z)
{
    c.set_limits({bl_x, bl_y, bl_z}, {tr_x, tr_y, tr_z});
}

void look_at(
    Camera &c,
    float px, float py, float pz,
    float ex, float ey, float ez,
    float ux, float uy, float uz)
{
    c.look_at({px, py, pz}, {ex, ey, ez}, {ux, uy, uz});
}

void register_camera(nngn::lua::table_view t) {
    t["new"] = [] { return Camera{}; };
    t["fov"] = fov;
    t["UPDATED"] = number(Camera::Flag::UPDATED);
    t["SCREEN_UPDATED"] = number(Camera::Flag::SCREEN_UPDATED);
    t["DASH"] = number(Camera::Flag::DASH);
    t["PERSPECTIVE"] = number(Camera::Flag::PERSPECTIVE);
    t["IGNORE_LIMITS"] = number(Camera::Flag::IGNORE_LIMITS);
    t["NEAR"] = number(Camera::NEAR);
    t["FAR"] = number(Camera::FAR);
    t["pos"] = [](const Camera &c) { return vec3(c.p); };
    t["vel"] = [](const Camera &c) { return vec3(c.v); };
    t["acc"] = [](const Camera &c) { return vec3(c.a); };
    t["rot"] = [](const Camera &c) { return vec3(c.rot); };
    t["rot_vel"] = [](const Camera &c) { return vec3(c.rot_v); };
    t["rot_acc"] = [](const Camera &c) { return vec3(c.rot_a); };
    t["bl_limit"] = [](const Camera &c) { return vec3(c.bl_limit); };
    t["tr_limit"] = [](const Camera &c) { return vec3(c.tr_limit); };
    t["zoom"] = [](const Camera &c) { return number(c.zoom); };
    t["zoom_vel"] = [](const Camera &c) { return number(c.zoom_v); };
    t["zoom_acc"] = [](const Camera &c) { return number(c.zoom_a); };
    t["max_vel"] = [](const Camera &c) { return number(c.max_v); };
    t["max_rot_vel"] = [](const Camera &c) { return number(c.max_rot_v); };
    t["max_zoom_vel"] = [](const Camera &c) { return number(c.max_zoom_v); };
    t["fov_y"] = [](const Camera &c) { return number(c.fov_y); };
    t["damp"] = [](const Camera &c) { return number(c.damp); };
    t["screen"] = screen;
    t["proj"] = [](const Camera &c) { return matrix(c.proj); };
    t["view"] = [](const Camera &c) { return matrix(c.view); };
    t["dash"] = &Camera::dash;
    t["perspective"] = &Camera::perspective;
    t["ignore_limits"] = ignore_limits;
    t["flags"] = [](const Camera &c) { return *c.flags; };
    t["set_dash"] = &Camera::set_dash;
    t["set_perspective"] = &Camera::set_perspective;
    t["set_ignore_limits"] = set_ignore_limits;
    t["center"] = get<&Camera::center>;
    t["up"] = get<&Camera::up>;
    t["world_to_view"] = world_to_view;
    t["view_to_clip"] = view_to_clip;
    t["clip_to_view"] = clip_to_view;
    t["view_to_world"] = view_to_world;
    t["z_for_fov"] = get<&Camera::z_for_fov>;
    t["scale_for_fov"] = get<&Camera::scale_for_fov>;
    t["set_screen"] = set_screen;
    t["set_pos"] = set_f<&Camera::set_pos>;
    t["set_vel"] = set<&Camera::v>;
    t["set_acc"] = set<&Camera::a>;
    t["set_rot"] = set_f<&Camera::set_rot>;
    t["set_rot_vel"] = set<&Camera::rot_v>;
    t["set_rot_acc"] = set<&Camera::rot_a>;
    t["set_zoom"] = &Camera::set_zoom;
    t["set_zoom_vel"] = [](Camera &c, float v) { c.zoom_v = v; };
    t["set_zoom_acc"] = [](Camera &c, float a) { c.zoom_a = a; };
    t["set_max_vel"] = [](Camera &c, float m) { c.max_v = m; };
    t["set_max_rot_vel"] = [](Camera &c, float m) { c.max_rot_v = m; };
    t["set_max_zoom_vel"] = [](Camera &c, float m) { c.max_zoom_v = m; };
    t["set_damp"] = [](Camera &c, float d) { c.damp = d; };
    t["set_fov_y"] = &Camera::set_fov_y;
    t["set_fov_z"] = &Camera::set_fov_z;
    t["set_limits"] = set_limits;
    t["look_at"] = look_at;
    t["update"] = &Camera::update;
}

}

NNGN_LUA_DECLARE_USER_TYPE(Camera)
NNGN_LUA_PROXY(Camera, register_camera)
