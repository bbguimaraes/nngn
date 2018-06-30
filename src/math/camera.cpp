#include <cassert>
#include <cmath>

#include "camera.h"

#include "timing/timing.h"

namespace nngn {

vec3 Camera::pos(void) const {
    vec3 ret = {0, 0, -1};
    ret = Math::rotate_x(ret, this->r.x);
    ret = Math::rotate_y(ret, this->r.y);
    ret = Math::rotate_z(ret, this->r.z);
    return ret + this->eye();
}

vec3 Camera::eye(void) const {
    auto ret = this->p;
    if(!this->flags.is_set(Flag::PERSPECTIVE))
        ret.z = this->fov_z() / this->zoom;
    return ret;
}

vec3 Camera::up(void) const {
    const auto dir = this->pos() - this->eye();
    vec3 lat = {1, 0, 0};
    lat = Math::rotate_x(lat, this->r.x);
    lat = Math::rotate_y(lat, this->r.y);
    lat = Math::rotate_z(lat, this->r.z);
    return Math::cross(lat, dir);
}

mat4 Camera::gen_proj(void) const {
    if(!this->screen.x || !this->screen.y)
        return mat4(1);
    auto s = static_cast<vec2>(this->screen);
    if(this->flags.is_set(Flag::PERSPECTIVE))
        return Math::perspective(this->fov_y, s.x / s.y, NEAR, FAR);
    s /= 2.0f;
    return Math::ortho(-s.x, s.x, -s.y, s.y, NEAR, FAR);
}

mat4 Camera::gen_hud_proj(void) const {
    const auto s = static_cast<vec2>(this->screen);
    return Math::ortho(0.0f, s.x, 0.0f, s.y);
}

mat4 Camera::gen_view(void) const {
    auto ret = mat4(1);
    ret = Math::scale(ret, {this->zoom, this->zoom, 1});
    if(this->r.x != 0)
        ret = Math::rotate(ret, this->r.x, {1, 0, 0});
    if(this->r.y != 0)
        ret = Math::rotate(ret, this->r.y, {0, 1, 0});
    if(this->r.z != 0)
        ret = Math::rotate(ret, this->r.z, {0, 0, 1});
    vec3 t = -this->p;
    if(!this->flags.is_set(Flag::PERSPECTIVE))
        t.z -= FAR / 2;
    ret = Math::translate(ret, t);
    return ret;
}

float Camera::fov_z(void) const {                 //   y
    return static_cast<float>(this->screen.y) // -----  tan(fovy/2) = y / 2 / z
        / 2.0f                                // \z| /  z = y / 2 / tan(fovy/2)
        / std::tan(this->fov_y / 2);          //  \|/
}                                             //  fovy

void Camera::set_perspective(bool b) {
    this->flags.set(Flag::PERSPECTIVE, b);
    const float d = this->fov_z();
    if(b) {
        this->p.z += d / this->zoom;
        this->zoom = 1;
    } else {
        this->zoom = d / this->p.z * this->zoom;
        this->p.z = 0;
    }
    this->flags.set(Flag::UPDATED);
    this->set_screen(this->screen);
}

void Camera::set_screen(const uvec2 &s) {
    this->screen = s;
    this->proj = this->gen_proj();
    this->hud_proj = this->gen_hud_proj();
    this->flags.set(Flag::SCREEN_UPDATED);
}

void Camera::set_pos(const vec3 &pos) {
    this->p = pos;
    this->flags.set(Flag::UPDATED);
}

void Camera::set_rot(const vec3 &rot) {
    this->r = rot;
    this->flags.set(Flag::UPDATED);
}

void Camera::set_zoom(float z) {
    this->zoom = z;
    this->flags.set(Flag::UPDATED);
}

void Camera::set_fov_y(float f) {
    this->fov_y = f;
    this->flags.set(Flag::UPDATED);
}

void Camera::look_at(const vec3 &pos, const vec3 &eye, const vec3 &up) {
    constexpr vec3 camera_dir = {0, 0, -1}, camera_lat = {1, 0, 0};
    auto w = Math::normalize(pos - eye);
    const auto w_xz = Math::normalize(vec3(w.x, 0, w.z));
    const auto ay = Math::angle(camera_dir, w_xz, up);
    w = Math::normalize(vec3(0, Math::rotate_y(w, -ay).yz()));
    const auto ax = Math::angle(camera_dir, w, camera_lat);
    this->p = eye;
    this->r = {ax, ay, 0};
    this->flags.set(Flag::UPDATED);
}

bool Camera::update(const Timing &t) {
    const float min_v = this->max_v / 512.0f;
    const float min_rv = this->max_rv / 512.0f;
    const float min_zv = this->max_zv / 512.0f;
    const float rot_mod = 0x1p-2f;
    const float zoom_mod = 0x1p-3f;
    const float dt = t.fdt_s();
    const float dash = this->dash() ? 3 : 1;
    const auto prev_v = this->v;
    const auto prev_rot_v = this->rv;
    const auto prev_zoom_v = this->zoom_v;
    const float damp_dt = 1.0f - this->damp * dt;
    if(this->a.x != 0 || this->a.y != 0 || this->a.z != 0)
        this->v += this->a * dash * dt;
    else {
        this->v *= damp_dt;
        if(Math::length2(this->v) < min_v * min_v)
            this->v = {0, 0, 0};
    }
    if(this->ra.x != 0 || this->ra.y != 0 || this->ra.z != 0)
        this->rv += this->ra * dash * dt;
    else {
        this->rv *= damp_dt;
        if(Math::length(this->rv) < min_rv * min_rv * rot_mod)
            this->rv = {0, 0, 0};
    }
    if(this->zoom_a != 0)
        this->zoom_v += this->zoom_a * dash * dt;
    else {
        this->zoom_v *= damp_dt;
        if(std::abs(this->zoom_v) < min_zv * zoom_mod)
            this->zoom_v = 0;
    }
    bool updated = this->flags.is_set(Flag::UPDATED | Flag::SCREEN_UPDATED);
    if(this->v.x != 0 || this->v.y != 0 || this->v.z != 0) {
        const float v_len = Math::length(this->v);
        const float max = this->max_v * dash;
        if(v_len > max)
            this->v *= max / v_len;
        this->p += (prev_v + this->v) * (dt / 2.0f);
        updated = true;
    }
    if(this->rv.x != 0 || this->rv.y != 0 || this->rv.z != 0) {
        const float v_len = Math::length(this->rv);
        const float max = this->max_rv * dash * rot_mod;
        if(v_len > max)
            this->rv *= max / v_len;
        this->r += (prev_rot_v + this->rv) * (dt / 2.0f);
        updated = true;
    }
    if(this->zoom_v != 0) {
        this->zoom_v = std::min(this->zoom_v, this->max_zv * dash * zoom_mod);
        this->zoom += (prev_zoom_v + this->zoom_v) * (dt / 2.0f);
        updated = true;
    }
    if(!updated)
        return false;
    this->view = this->gen_view();
    this->proj = this->gen_proj();
    this->flags.clear(Flag::UPDATED | Flag::SCREEN_UPDATED);
    return true;
}

}
