/**
 * \dir src/math
 * \brief Abstract mathematical utilities.
 */
#ifndef NNGN_MATH_MATH_H
#define NNGN_MATH_MATH_H

#include <bit>
#include <cassert>
#include <cmath>
#include <optional>
#include <random>

#include "utils/concepts/fundamental.h"

#include "mat4.h"

namespace nngn {

class Math {
public:
    struct rnd_generator_t : private std::mt19937 {
        using std::mt19937::default_seed;
        using std::mt19937::result_type;
        using std::mt19937::min;
        using std::mt19937::max;
        using std::mt19937::mt19937;
        using std::mt19937::operator();
        using std::mt19937::operator=;
        using std::mt19937::seed;
        rnd_generator_t();
    };
    using rand_seed_t = decltype(rnd_generator_t::default_seed);
private:
    std::optional<rnd_generator_t> m_rnd_generator = {};
public:
    // Constants
    template<typename T> static constexpr T sq2_2();
    template<typename T> static constexpr T sq2();
    template<typename T> static constexpr T e();
    template<typename T> static constexpr T pi();
    template<typename T> static constexpr T tau();
    template<typename T> static constexpr T radians(T d);
    template<typename T> static constexpr T degrees(T r);
    // Scalar
    template<typename T> static constexpr T round_up(T n, T d);
    template<unsigned_integral T> static constexpr T mip_levels(T extent);
    // Vector
    template<typename T> static constexpr auto length2(const T &v);
    template<typename T> static auto length(const T &v);
    template<typename T> static T normalize(const T &v);
    template<template<typename> typename V, typename T>
    requires vector<V, T>
    static V<T> clamp_len(V<T> v, T len);
    template<typename T>
    static constexpr vector_type<T> sum(const T &v);
    template<typename T>
    static constexpr vector_type<T> product(const T &v);
    template<typename T>
    static constexpr vector_type<T> avg(const T &v);
    template<typename T>
    static constexpr vector_type<T> dot(const T &u, const T &v);
    template<typename T> static constexpr vec3_base<T> cross(
        const vec3_base<T> &u, const vec3_base<T> &v);
    template<typename T>
    static constexpr T angle(const vec2_base<T> &u, const vec2_base<T> &v);
    template<typename T>
    static constexpr T angle(
        const vec3_base<T> &u, const vec3_base<T> &v, const vec3_base<T> &n);
    template<typename T>
    static constexpr vec3_base<T> reflect(
        const vec3_base<T> &v, const vec3_base<T> &n);
    // Plane
    template<typename T> static constexpr vec3_base<T> normal(
        const vec3_base<T> &p0, const vec3_base<T> &p1, const vec3_base<T> &p2);
    // Matrix
    static void mat_mul(size_t n, const float *m0, const float *m1, float *m2);
    template<typename T>
    static constexpr mat4_base<T> transpose(const mat4_base<T> &m);
    template<typename T>
    static constexpr mat4_base<T> translate(
        const mat4_base<T> &m, const vec3_base<T> &v);
    template<typename T>
    static constexpr mat4_base<T> scale(
        const mat4_base<T> &m, const vec3_base<T> &v);
    // Rotation
    template<typename T>
    static constexpr vec2_base<T> rotate(const vec2_base<T> &v, T sin, T cos);
    template<typename T>
    static constexpr vec2_base<T> rotate(const vec2_base<T> &v, T angle);
    template<typename T>
    static constexpr vec3_base<T> rotate(
        const vec3_base<T> &v, T sin, T cos, const vec3_base<T> &n);
    template<typename T>
    static constexpr vec3_base<T> rotate(
        const vec3_base<T> &v, T angle, const vec3_base<T> &n);
    template<typename T>
    static constexpr vec3_base<T> rotate_x(const vec3_base<T> &v, T sin, T cos);
    template<typename T>
    static constexpr vec3_base<T> rotate_y(const vec3_base<T> &v, T sin, T cos);
    template<typename T>
    static constexpr vec3_base<T> rotate_z(const vec3_base<T> &v, T sin, T cos);
    template<typename T>
    static constexpr vec3_base<T> rotate_x(const vec3_base<T> &v, T angle);
    template<typename T>
    static constexpr vec3_base<T> rotate_y(const vec3_base<T> &v, T angle);
    template<typename T>
    static constexpr vec3_base<T> rotate_z(const vec3_base<T> &v, T angle);
    template<typename T>
    static constexpr mat4_base<T> rotate(
        const mat4_base<T> &m, T angle, const vec3_base<T> &v);
    // Projection
    template<typename T>
    static constexpr mat4_base<T> ortho(T left, T right, T bottom, T top);
    template<typename T>
    static constexpr mat4_base<T> ortho(
        T left, T right, T bottom, T top, T near, T far);
    template<typename T>
    static constexpr mat4_base<T> perspective(T fovy, T aspect, T near, T far);
    // View
    template<typename T>
    static constexpr mat4_base<T> look_at(
        const vec3_base<T> &eye, const vec3_base<T> &center,
        const vec3_base<T> &up);
    // Etc.
    static void gaussian_filter(std::size_t size, float std_dev, float *p);
    static void gaussian_filter(
        std::size_t xsize, std::size_t ysize, float std_dev, float *p);
    auto *rnd_generator() { return &*this->m_rnd_generator; }
    void init();
    void seed_rand(rand_seed_t s);
    void rand_mat(size_t n, float *m);
};

template<typename T> inline constexpr T Math::sq2()
    { return static_cast<T>(1.41421356237309514547462185873882845); }
template<typename T> inline constexpr T Math::sq2_2()
    { return static_cast<T>(0.70710678118654757273731092936941423); }
template<typename T> inline constexpr T Math::e()
    { return static_cast<T>(2.7182818284590450907955982984276488); }
template<typename T> inline constexpr T Math::pi()
    { return static_cast<T>(3.14159265358979323846264338327950288); }
template<typename T> inline constexpr T Math::tau()
    { return static_cast<T>(6.28318530717958647692528676655900576); }

template<typename T> inline constexpr T Math::radians(T d)
    { constexpr auto mul = Math::tau<T>() / T{360}; return d * mul; }
template<typename T> inline constexpr T Math::degrees(T r)
    { constexpr auto mul = T{360} * Math::tau<T>(); return r * mul; }

template<typename T> constexpr T Math::round_up(T n, T d) {
    const auto r = n % d;
    return r ? n + d - r : n;
}

template<unsigned_integral T>
constexpr T Math::mip_levels(T extent) {
    assert(std::popcount(extent) == 1);
    return T{1} + static_cast<T>(std::countr_zero(std::bit_floor(extent)));
}

template<typename T> inline constexpr auto Math::length2(const T &v)
    { return Math::dot(v, v); }
template<typename T> inline auto Math::length(const T &v)
    { return std::sqrt(Math::length2(v)); }
template<typename T> inline T Math::normalize(const T &v)
    { return v / Math::length(v); }

template<template<typename> typename V, typename T>
requires vector<V, T>
inline V<T> Math::clamp_len(V<T> v, T len) {
    const auto len2 = Math::length2(v), max2 = len * len;
    return len2 <= max2 ? v : v * std::sqrt(max2 / len2);
}

template<typename T>
inline constexpr vector_type<T> Math::sum(const T &v) {
    return std::accumulate(begin(v), end(v), vector_type<T>{});
}

template<typename T>
inline constexpr vector_type<T> Math::product(const T &v) {
    return std::accumulate(
        begin(v), end(v), vector_type<T>{1}, std::multiplies<>{});
}

template<typename T>
inline constexpr vector_type<T> Math::avg(const T &v)
    { return Math::sum(v) / vector_type<T>{T::n_dim}; }

template<typename T>
inline constexpr vector_type<T> Math::dot(const T &u, const T &v) {
    return std::inner_product(begin(u), end(u), begin(v), vector_type<T>{});
}

template<typename T>
inline constexpr vec3_base<T> Math::cross(
        const vec3_base<T> &u, const vec3_base<T> &v) {
    return {
        u.y * v.z - u.z * v.y,
        u.z * v.x - u.x * v.z,
        u.x * v.y - u.y * v.x};
}

template<typename T>
inline constexpr T Math::angle(
    const vec2_base<T> &u, const vec2_base<T> &v
) {
    return Math::angle(vec3_base<T>(u), vec3_base<T>(v), {0, 0, 1});
}

template<typename T>
inline constexpr T Math::angle(
    const vec3_base<T> &u, const vec3_base<T> &v, const vec3_base<T> &n
) {
    const auto cross = Math::cross(u, v);
    const auto ret = std::acos(Math::dot(u, v));
    return Math::dot(cross, n) < 0 ? Math::tau<T>() - ret : ret;
}

template<typename T>
inline constexpr vec3_base<T> Math::reflect(
    const vec3_base<T> &v, const vec3_base<T> &n
) {
    return v - T(2) * n * Math::dot(v, n);
}

template<typename T>
inline constexpr vec3_base<T> Math::normal(
    const vec3_base<T> &p0, const vec3_base<T> &p1, const vec3_base<T> &p2
) {
    return Math::cross(p1 - p0, p2 - p0);
}

template<typename T>
inline constexpr mat4_base<T> Math::transpose(const mat4_base<T> &m) {
    const auto r0 = m[0], r1 = m[1], r2 = m[2], r3 = m[3];
    return {
        r0[0], r1[0], r2[0], r3[0],
        r0[1], r1[1], r2[1], r3[1],
        r0[2], r1[2], r2[2], r3[2],
        r0[3], r1[3], r2[3], r3[3]};
}

template<typename T>
inline constexpr mat4_base<T> Math::translate(
        const mat4_base<T> &m, const vec3_base<T> &v) {
    return {
        m[0], m[1], m[2],
        {m[0] * v[0] + m[1] * v[1] + m[2] * v[2] + m[3]}};
}

template<typename T>
inline constexpr mat4_base<T> Math::scale(
        const mat4_base<T> &m, const vec3_base<T> &v)
    { return {m[0] * v[0], m[1] * v[1], m[2] * v[2], m[3]}; }

template<typename T>
inline constexpr vec2_base<T> Math::rotate(const vec2_base<T> &v, T sin, T cos)
    { return {v.x * cos - v.y * sin, v.x * sin + v.y * cos}; }

template<typename T>
inline constexpr vec2_base<T> Math::rotate(const vec2_base<T> &v, T angle)
    { return Math::rotate(v, std::sin(angle), std::cos(angle)); }

template<typename T>
inline constexpr vec3_base<T> Math::rotate(
        const vec3_base<T> &v, T sin, T cos, const vec3_base<T> &n) {
    const auto proj = n * Math::dot(n, v);
    const auto d = v - proj;
    const auto rot = cos * d + sin * Math::cross(n, d);
    return proj + rot;
}

template<typename T>
inline constexpr vec3_base<T> Math::rotate(
        const vec3_base<T> &v, T angle, const vec3_base<T> &n)
    { return Math::rotate(v, std::sin(angle), std::cos(angle), n); }

template<typename T>
inline constexpr vec3_base<T> Math::rotate_x(
        const vec3_base<T> &v, T sin, T cos)
    { return {v.x, Math::rotate(v.yz(), sin, cos)}; }

template<typename T>
inline constexpr vec3_base<T> Math::rotate_y(
        const vec3_base<T> &v, T sin, T cos) {
    const auto ret = Math::rotate(v.zx(), sin, cos);
    return {ret[1], v.y, ret[0]};
}

template<typename T>
inline constexpr vec3_base<T> Math::rotate_z(
        const vec3_base<T> &v, T sin, T cos)
    { return {Math::rotate(v.xy(), sin, cos), v.z}; }

template<typename T>
inline constexpr vec3_base<T> Math::rotate_x(const vec3_base<T> &v, T angle)
    { return Math::rotate_x(v, std::sin(angle), std::cos(angle)); }

template<typename T>
inline constexpr vec3_base<T> Math::rotate_y(const vec3_base<T> &v, T angle)
    { return Math::rotate_y(v, std::sin(angle), std::cos(angle)); }

template<typename T>
inline constexpr vec3_base<T> Math::rotate_z(const vec3_base<T> &v, T angle)
    { return Math::rotate_z(v, std::sin(angle), std::cos(angle)); }

template<typename T>
inline constexpr mat4_base<T> Math::rotate(
        const mat4_base<T> &m, T angle, const vec3_base<T> &v) {
    const T cos = std::cos(angle);
    const T sin = std::sin(angle);
    const auto axis = Math::normalize(v);
    const auto tmp = (T{1} - cos) * axis;
    mat4 rot = {};
    rot[0][0] = cos + tmp[0] * axis[0];
    rot[0][1] = tmp[1] * axis[0] - sin * axis[2];
    rot[0][2] = tmp[2] * axis[0] + sin * axis[1];
    rot[1][0] = tmp[0] * axis[1] + sin * axis[2];
    rot[1][1] = cos + tmp[1] * axis[1];
    rot[1][2] = tmp[2] * axis[1] - sin * axis[0];
    rot[2][0] = tmp[0] * axis[2] - sin * axis[1];
    rot[2][1] = tmp[1] * axis[2] + sin * axis[0];
    rot[2][2] = cos + tmp[2] * axis[2];
    rot[3][3] = T{1};
    return m * rot;
}

template<typename T>
inline constexpr mat4_base<T> Math::ortho(T left, T right, T bottom, T top) {
    const auto w = right - left, h = top - bottom;
    return Math::transpose(mat4_base<T>{
        T{2} / w, 0, 0, -(right + left) / w,
        0, T{2} / h, 0, -(top + bottom) / h,
        0, 0, T{-1}, 0,
        0, 0, 0, T{1}});
}

template<typename T>
inline constexpr mat4_base<T> Math::ortho(
        T left, T right, T bottom, T top, T near, T far) {
    auto ret = Math::ortho(left, right, bottom, top);
    const auto z = far - near;
    ret[2][2] = -T{2} / z;
    ret[3][2] = -(far + near) / z;
    return ret;
}

template<typename T>
inline constexpr mat4_base<T> Math::perspective(
        T fovy, T aspect, T near, T far) {
    assert(std::abs(aspect) > -std::numeric_limits<T>::epsilon());
    const auto z = far - near;
    const auto t = std::tan(fovy / T{2});
    return Math::transpose(mat4_base<T>{
        T{1} / (aspect * t), 0, 0, 0,
        0, T{1} / t, 0, 0,
        0, 0, -(far + near) / z, -(T{2} * far * near) / z,
        0, 0, -T{1}, 0});
}

template<typename T>
inline constexpr mat4_base<T> Math::look_at(
        const vec3_base<T> &eye, const vec3_base<T> &center,
        const vec3_base<T> &up) {
    const auto f = Math::normalize(center - eye);
    const auto s = Math::normalize(Math::cross(f, up));
    const auto u = Math::cross(s, f);
    return Math::transpose(mat4{
        vec4{s, -Math::dot(s, eye)},
        vec4{u, -Math::dot(u, eye)},
        vec4{-f, Math::dot(f, eye)},
        vec4{0, 0, 0, 1}});
}

}

#endif
