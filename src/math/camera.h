#ifndef NNGN_CAMERA_H
#define NNGN_CAMERA_H

#include <bitset>
#include <vector>

#include "const.h"

#include "utils/flags.h"

#include "mat4.h"
#include "math.h"
#include "vec2.h"
#include "vec3.h"

namespace nngn {

struct Timing;

struct Camera {
    enum Flag : uint8_t {
        UPDATED = 1u << 1,
        SCREEN_UPDATED = 1u << 2,
        DASH = 1u << 3,
        PERSPECTIVE = 1u << 4,
        N_FLAGS = 1u << 5,
    };
    static constexpr float NEAR = 0.01f, FAR = 2048.0f;
    static constexpr float FOVY = Math::radians(60.0f);
    static const float TAN_FOVY;
    static const float TAN_HALF_FOVY;
    vec3 p = {}, v = {}, a = {};
    vec3 r = {}, rv = {}, ra = {};
    float zoom = 1, zoom_v = {}, zoom_a = {};
    uvec2 screen = {};
    mat4 proj = mat4(1), hud_proj = mat4(1), view = mat4(1);
    float max_v = std::numeric_limits<float>::infinity();
    float max_rv = std::numeric_limits<float>::infinity();
    float max_zv = std::numeric_limits<float>::infinity();
    float damp = {};
    Flags<Flag> flags = {};
    bool dash() const { return this->flags.is_set(Flag::DASH); }
    bool perspective() const { return this->flags.is_set(Flag::PERSPECTIVE); }
    vec3 pos() const;
    vec3 eye() const;
    mat4 gen_proj() const;
    mat4 gen_hud_proj() const;
    mat4 gen_view() const;
    float fov_z() const;
    void set_perspective(bool b);
    void set_screen(const uvec2 &s);
    void set_pos(const vec3 &pos);
    void set_rot(const vec3 &rot);
    void set_zoom(float z);
    void set_dash(bool b) { this->flags.set(Flag::DASH, b); }
    void look_at(const vec3 &pos, const vec3 &eye, const vec3 &up);
    bool update(const Timing &t);
};

}

#endif
