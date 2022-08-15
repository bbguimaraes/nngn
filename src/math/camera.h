#ifndef NNGN_MATH_CAMERA_H
#define NNGN_MATH_CAMERA_H

#include "utils/def.h"
#include "utils/flags.h"

#include "mat4.h"
#include "math.h"
#include "vec2.h"
#include "vec3.h"

namespace nngn {

struct Timing;

/**
 * Abstract orthographic/perspective camera.
 *
 * Two complementary projection modes are supported: orthographic and
 * perspective.  Some facilities are provided to make them as close to each
 * other as possible.
 *
 * ### Orthographic projection
 *
 * In orthographic mode, objects ocupy the XY plane.  The projection matrix is
 * set up so that objects are displayed in their natural size, scaled by the
 * \ref zoom parameter.  Their Z position is expected to be set such that they
 * can be rendered in any order, using a depth buffer to sort overlaps.
 *
 * The `z` value of the position is also used as a scaling factor.  This
 * preserves the size of objects on the XY plane when transitioning from
 * ortographic to perspective mode, as long as the camera remains parallel to
 * the Z axis.  The scaling factor is the ratio of \ref z_for_fov to the `z`
 * position.
 *
 * ### Perspective projection
 *
 * In perspective mode, objects maintain their position in the XY plane but have
 * a third dimension in the Z direction.  A standing sprite thus has the same XY
 * position but is rotated 90° at its base around the X axis.  Although
 * counter-intuitive, having the "up" vector parallel to the Z axis (as opposed
 * to the Y axis, as is commonly done) allows sprites to occupy the same XY
 * position in both projection modes.
 *
 * This mode tries to be as close to the orthographic mode as possible, by
 * adjusting the camera's projection matrix and Z position so that the position
 * and relative scale of objects is maintained as long as the camera view vector
 * is parallel to the Z axis.  With the regular camera orientation facing the
 * direction of the -Z axis, toggling between the two projection modes leaves
 * objects in the XY plane (such as the map) unchanged.
 *
 * ### Field of view
 *
 * Calculations of dimensions, field-of-view, etc. are based on a 14"/35.5cm,
 * 16:9 laptop monitor at a distance of 60cm from the camera/eye:
 *
 *      x/y
 *     -----  x = 31cm
 *     \z| /  y = 17.4cm
 *      \|/   z = 60cm
 *      fov   fov = 2 * atan(31/60) ~= 60° = π/3
 *
 * The initial size of the window is taken to be the full screen size and
 * assigned a vertical field-of-view angle of 60° (π/3).  This angle can be
 * automatically adjusted when the window is resized (see \ref set_fov_z): this
 * ensures the image is not distorted --- i.e. objects retain their size and
 * proportion --- in perspective mode.
 */
struct Camera {
    /** State flags and configuration. */
    enum Flag : u8 {
        /**
         * Parameters of the camera have changed.
         * This indicates the matrices need to be recalculated (via \ref
         * update).
         */
        UPDATED        = 1u << 0,
        /**
         * The screen size has changed.
         * Not used internally, but can be used by external code mid-frame to
         * determine whether values dependent on the screen size need to be
         * updated.
         */
        SCREEN_UPDATED = 1u << 1,
        /** Increases the speed of displacement/rotation/etc. */
        DASH           = 1u << 2,
        /** Chooses between orthographic/perspective mode. */
        PERSPECTIVE    = 1u << 3,
        /** The camera limits do not restrict movement when set. */
        IGNORE_LIMITS  = 1u << 4,
    };
    /** Perspective projection near plane. */
    static constexpr float NEAR = 0.01f;
    /** Perspective projection far plane. */
    static constexpr float FAR = 2048;
    /** Perspective projection field of view along the Y axis (default). */
    static constexpr float FOVY = static_cast<float>(Math::pi() / 3);
    /**
     * Calculates field of view based on screen size and distance.
     *
     *       w
     *     -----  a = fov / 2, op = w / 2, adj = z
     *     \z| /  tan(a) = op / adj
     *      \|/   tan(fov/2) = w / 2 / z
     *      fov
     *
     * \param z Distance from the camera to the projection plane.
     * \param w Width of the projection plane.
     * \returns
     *     Angle such that the intersection of the field of view with a
     *     line/plane at distance \p z has width \p w.
     */
    static constexpr float fov(float z, float w);
    // Flags
    bool dash(void) const;
    bool perspective(void) const;
    void set_dash(bool b);
    void set_perspective(bool b);
    void set_ignore_limits(bool b);
    // Positions / vectors
    /** \ref p added to a unit vector in the view direction. */
    vec3 center(void) const;
    /** The "up" vector for the view direction. */
    vec3 up(void) const;
    /** Projects a point from world to view space. */
    vec3 world_to_view(vec3 p) const;
    /** Projects a point from view to clip space. */
    vec3 view_to_clip(vec3 p) const;
    /** Projects a point from clip to screen space. */
    vec2 clip_to_screen(vec3 p);
    /** Projects a point from screen to clip space. */
    vec3 screen_to_clip(vec2 p);
    /** Projects a point from clip to view space. */
    vec3 clip_to_view(vec3 p) const;
    /** Projects a point from view to world space. */
    vec3 view_to_world(vec3 p) const;
    // Screen
    /**
     * Derives Z coordinate from the camera's screen size and field of view.
     * When changing the camera's projection matrix from perspective to
     * orthographic, using this Z coordinate will preserve the size of objects
     * directly on the XY plane (provided the camera's view vector is
     * perpendicular to it, i.e. parallel to the Z axis).
     *
     *       y
     *     -----   a = fov / 2, op = y / 2, adj = z
     *     \z| /   tan(a) = op / adj
     *      \|/    tan(fov / 2) = y / 2 / z
     *      fov    z = y / 2 / tan(fov / 2)
     */
    float z_for_fov(void) const;
    /**
     * Derives orthographic scaling factor from camera parameters.
     * When an orthographic projection is used (i.e. when the camera's Z
     * position does not alter the apparent size of objects), this factor can be
     * used to preserve the size of objects directly on the XY plane as if a
     * perspective projection were used (with the same constraints as \ref
     * z_for_fov).
     *
     *        y'
     *     ---------  y = screen.y
     *     \ z'|   /  z = z_for_fov(), z' = p.z
     *     y\-----/   y' / z' = y / z
     *       \z| /    s = y / y'
     *        \|/     s = z / z'
     *        fov
     */
    float scale_for_fov(void) const;
    // Mutators
    void set_pos(vec3 p);
    void set_rot(vec3 r);
    void set_zoom(float z);
    void set_fov_y(float f);
    void set_fov_z(float z) { this->fov_z = z; }
    void set_screen(uvec2 s);
    void set_limits(vec3 bl, vec3 tr);
    void look_at(vec3 center, vec3 pos, vec3 up);
    bool update(const Timing &t);
    // Data
    /** Position. */
    vec3 p = {};
    /** Velocity. */
    vec3 v = {};
    /** Acceleration. */
    vec3 a = {};
    /** Rotational Euler angles (ZYX). */
    vec3 rot = {};
    /** Rotational velocity. */
    vec3 rot_v = {};
    /** Rotational acceleration. */
    vec3 rot_a = {};
    /** Bottom left point of the limit for \ref p. */
    vec3 bl_limit = {-INFINITY, -INFINITY, -INFINITY};
    /** Top right point of the limit for \ref p. */
    vec3 tr_limit = {+INFINITY, +INFINITY, +INFINITY};
    /** Scaling factor. */
    float zoom = 1;
    /** Scaling factor velocity. */
    float zoom_v = {};
    /** Scaling factor acceleration. */
    float zoom_a = {};
    /** Maximum value for \ref v. */
    float max_v = INFINITY;
    /** Maximum value for \ref rot_v. */
    float max_rot_v = INFINITY;
    /** Maximum value for \ref zoom_v. */
    float max_zoom_v = INFINITY;
    /** Field of view in perspective mode. */
    float fov_y = FOVY;
    /**
     * Enable auto-adjustment of the field-of-view angle.
     * In perspective mode, this makes the image retain its size when the
     * window is resized.  The value should be \ref z_for_fov.  A value of zero
     * disables auto-adjustment.
     */
    float fov_z = {};
    /** Damping factor when not accelerating. */
    float damp = {};
    /** Screen size. */
    uvec2 screen = {};
    /** Projection matrix (orthographic/perspective). */
    mat4 proj = mat4{1};
    /** Projection matrix for screen coordinates. */
    mat4 screen_proj = mat4{1};
    /** View matrix. */
    mat4 view = mat4{1};
    /** Inverse matrix of \ref proj. */
    mat4 inv_proj = mat4{1};
    /** Inverse matrix of \ref view. */
    mat4 inv_view = mat4{1};
    Flags<Flag> flags = {};
};

inline constexpr float Camera::fov(float z, float w) {
    return 2.0f * std::atan(w / 2.0f / z);
}

inline bool Camera::dash(void) const {
    return this->flags.is_set(Flag::DASH);
}

inline bool Camera::perspective(void) const {
    return this->flags.is_set(Flag::PERSPECTIVE);
}

inline void Camera::set_dash(bool b) {
    this->flags.set(Flag::DASH, b);
}

inline void Camera::set_perspective(bool b) {
    this->flags.set(Flag::PERSPECTIVE, b);
    this->flags.set(Flag::UPDATED);
}

inline void Camera::set_ignore_limits(bool b) {
    this->flags.set(Flag::IGNORE_LIMITS, b);
    this->flags.set(Flag::UPDATED);
}

inline vec3 Camera::world_to_view(vec3 wp) const {
    return Math::perspective_transform(this->view, wp);
}

inline vec3 Camera::view_to_clip(vec3 vp) const {
    return Math::perspective_transform(this->proj, vp);
}

inline vec2 Camera::clip_to_screen(vec3 cp) {
    return (cp.xy() + vec2{1}) / 2.0f * static_cast<vec2>(this->screen);
}

inline vec3 Camera::screen_to_clip(vec2 sp) {
    return {2.0f * (sp / static_cast<vec2>(this->screen)) - vec2{1} , 0};
}

inline vec3 Camera::clip_to_view(vec3 cp) const {
    return Math::perspective_transform(this->inv_proj, cp);
}

inline vec3 Camera::view_to_world(vec3 vp) const {
    return Math::perspective_transform(this->inv_view, vp);
}

inline float Camera::z_for_fov(void) const {
    return static_cast<float>(this->screen.y)
        / 2.0f
        / std::tan(this->fov_y / 2);
}

inline float Camera::scale_for_fov(void) const {
    return this->zoom * this->z_for_fov() / this->p.z;
}

inline void Camera::set_pos(vec3 pos) {
    this->p = pos;
    this->flags.set(Flag::UPDATED);
}

inline void Camera::set_rot(vec3 r) {
    this->rot = r;
    this->flags.set(Flag::UPDATED);
}

inline void Camera::set_zoom(float z) {
    this->zoom = z;
    this->flags.set(Flag::UPDATED);
}

inline void Camera::set_fov_y(float f) {
    this->fov_y = f;
    this->flags.set(Flag::UPDATED);
}

inline void Camera::set_screen(uvec2 s) {
    this->screen = s;
    this->flags.set(Flag::SCREEN_UPDATED);
}

inline void Camera::set_limits(vec3 bl, vec3 tr) {
    this->bl_limit = bl;
    this->tr_limit = tr;
    this->flags.set(Flag::UPDATED);
}

}

#endif
