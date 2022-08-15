#include <cassert>
#include <cmath>

#include "camera.h"

#include "timing/timing.h"

namespace {

using nngn::vec3, nngn::Math;

vec3 rotate(vec3 v, vec3 rot) {
    v = Math::rotate_x(v, rot.x);
    v = Math::rotate_y(v, rot.y);
    v = Math::rotate_z(v, rot.z);
    return v;
}

}

namespace nngn {

vec3 Camera::center(void) const {
    return rotate({0, 0, -1}, this->rot) + this->p;
}

vec3 Camera::up(void) const {
    const auto dir = rotate({0, 0, -1}, this->rot);
    const auto lat = rotate({1, 0,  0}, this->rot);
    return Math::cross(lat, dir);
}

void Camera::look_at(vec3 center, vec3 pos, vec3 up) {
    constexpr vec3 camera_dir = {0, 0, -1};
    constexpr vec3 camera_lat = {1, 0, 0};
    constexpr vec3 camera_up = {0, 1, 0};
    const auto w0 = Math::normalize(center - pos);
    const auto w_xz = Math::normalize(vec3{w0.x, 0.0f, w0.z});
    const auto ay = Math::angle(camera_dir, w_xz, up);
    const auto w1 = Math::normalize(vec3{0.0f, Math::rotate_y(w0, -ay).yz()});
    const auto ax = Math::angle(camera_dir, w1, camera_lat);
    const auto az = Math::angle(camera_up, up, -camera_dir);
    this->p = pos;
    this->rot = {ax, ay, az};
    this->flags.set(Flag::UPDATED);
}

bool Camera::update(const Timing &t) {
    constexpr auto update_v = []<typename T, typename M>(
        T &dst, T src, M min, M max,
        float dt, float damp_dt, float dash)
    {
        if(src == T{})
            dst *= damp_dt;
        else
            dst += src * dash * dt;
        if(const auto len2 = Math::length2(dst); len2 < min * min)
            dst = {};
        else {
            const float len = std::sqrt(len2);
            if(const auto m = max * dash; m < len)
                dst *= m / len;
        }
    };
    constexpr auto update_p = []<typename T>(T &dst, T src, T prev, float dt) {
        if(src == T{})
            return false;
        dst += (prev + src) * (dt / 2.0f);
        return true;
    };
    constexpr auto gen_proj = [](const Camera &c) {
        if(c.screen == uvec2{})
            return mat4{1};
        auto s = static_cast<vec2>(c.screen);
        if(c.flags.is_set(Flag::PERSPECTIVE))
            return Math::perspective(
                c.fov_y, s.x / s.y, Camera::NEAR, Camera::FAR);
        s /= 2.0f;
        const auto z = c.p.z + c.p.y, far_2 = Camera::FAR / 2;
        return Math::ortho(-s.x, s.x, -s.y, s.y, z - far_2, z + far_2);
    };
    constexpr auto gen_screen_proj = [](const Camera &c) {
        const auto s = static_cast<vec2>(c.screen);
        return Math::ortho(0.0f, s.x, 0.0f, s.y);
    };
    constexpr auto gen_view = [](const Camera &c) {
        const auto scale = [&c](mat4 m) {
            const bool persp = c.flags.is_set(Flag::PERSPECTIVE);
            const float s = c.zoom * (persp ? 1 : (c.z_for_fov() / c.p.z));
            return Math::scale(m, {s, s, 1});
        };
        const auto rotate = [&c](mat4 m) {
            const auto r = c.rot;
            if(r.x != 0)
                m = Math::rotate(m, -r.x, {1, 0, 0});
            if(r.y != 0)
                m = Math::rotate(m, -r.y, {0, 1, 0});
            if(r.z != 0)
                m = Math::rotate(m, -r.z, {0, 0, 1});
            return m;
        };
        const auto translate = [&c](mat4 m) {
            if(c.flags.is_set(Flag::IGNORE_LIMITS))
                return m;
            const auto d = static_cast<vec2>(c.screen) / (c.zoom * 2.0f);
            const vec3 bl = c.bl_limit + vec3{d, 0};
            const vec3 tr = c.tr_limit - vec3{d, 0};
            return Math::translate(m, -vec3{
                std::clamp(c.p[0], bl[0], tr[0]),
                std::clamp(c.p[1], bl[1], tr[1]),
                std::clamp(c.p[2], bl[2], tr[2]),
            });
        };
        return translate(rotate(scale(mat4{1})));
    };
    const float dt = t.fdt_s();
    const float damp_dt = 1.0f - this->damp * dt;
    const float dash = this->dash() ? 3 : 1;
    constexpr float min = 0x1p-9f;
    constexpr float rot_mod = 0x1p-3f;
    constexpr float zoom_mod = 0x1p-3f;
    const auto prev_v = this->v;
    const auto prev_rot_v = this->rot_v;
    const auto prev_zoom_v = this->zoom_v;
    update_v(
        this->v, this->a,
        this->max_v * min, this->max_v,
        dt, damp_dt, dash);
    update_v(
        this->rot_v, this->rot_a,
        this->max_rot_v * min * rot_mod, this->max_rot_v,
        dt, damp_dt, dash);
    update_v(
        this->zoom_v, this->zoom_a,
        this->max_zoom_v * min * zoom_mod, this->max_zoom_v,
        dt, damp_dt, dash);
    const bool screen_updated = this->flags.is_set(Flag::SCREEN_UPDATED);
    if(screen_updated && this->fov_z != 0)
        this->fov_y =
            Camera::fov(this->fov_z, static_cast<float>(this->screen.y));
    bool updated = screen_updated || this->flags.is_set(Flag::UPDATED);
    if(update_p(this->p, this->v, prev_v, dt))
        updated = true;
    if(update_p(this->rot, this->rot_v, prev_rot_v, dt))
        updated = true;
    if(update_p(this->zoom, this->zoom_v, prev_zoom_v, dt))
        updated = true;
    if(!updated)
        return false;
    this->view = gen_view(*this);
    this->proj = gen_proj(*this);
    this->screen_proj = gen_screen_proj(*this);
    this->inv_view = Math::inverse(this->view);
    this->inv_proj = Math::inverse(this->proj);
    this->flags.clear(Flag::UPDATED | Flag::SCREEN_UPDATED);
    return true;
}

}
