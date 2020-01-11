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
#include <span>

#include "utils/concepts/fundamental.h"

#include "mat3.h"
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
        rnd_generator_t(void);
    };
    using rand_seed_t = std::decay_t<decltype(rnd_generator_t::default_seed)>;
private:
    std::optional<rnd_generator_t> m_rnd_generator = {};
public:
    // Constants
    template<typename T = double> static constexpr T sq2_2(void);
    template<typename T = double> static constexpr T sq2(void);
    template<typename T = double> static constexpr T e(void);
    template<typename T = double> static constexpr T pi(void);
    template<typename T = double> static constexpr T tau(void);
    template<typename T = double> static constexpr T radians(T d);
    template<typename T = double> static constexpr T degrees(T r);
    // Scalar
    template<typename T> static constexpr T round_down_pow2(T n, T d);
    template<typename T> static constexpr T round_up_pow2(T n, T d);
    template<typename T> static constexpr T round_down(T n, T d);
    template<typename T> static constexpr T round_up(T n, T d);
    template<typename T> static constexpr T round_up_div(T n, T d);
    template<unsigned_integral T> static constexpr T mip_levels(T extent);
    template<typename T> static constexpr auto dot(T u, T v);
    // Vector
    template<typename T> static constexpr auto length2(T v);
    template<typename T> static auto length(T v);
    template<typename T> static T normalize(T v);
    template<template<typename> typename V, typename T>
    requires vector<V, T>
    static V<T> clamp_len(V<T> v, T len);
    template<typename T>
    static constexpr vector_type<T> sum(T v);
    template<typename T>
    static constexpr vector_type<T> product(T v);
    template<typename T>
    static constexpr vector_type<T> avg(T v);
    template<typename T>
    static constexpr vec3_base<T> cross(vec3_base<T> u, vec3_base<T> v);
    template<typename T>
    static constexpr T angle(vec2_base<T> u, vec2_base<T> v);
    template<typename T>
    static constexpr T angle(vec3_base<T> u, vec3_base<T> v, vec3_base<T> n);
    template<typename T>
    static constexpr vec3_base<T> reflect(vec3_base<T> v, vec3_base<T> n);
    // Plane
    template<typename T>
    static constexpr vec3_base<T> normal(
        vec3_base<T> p0, vec3_base<T> p1, vec3_base<T> p2);
    // Matrix
    static void mat_mul(
        std::span<float> dst, const float *src0, const float *src1,
        std::size_t n);
    template<typename T>
    static vec3_base<T> perspective_transform(
        const mat4_base<T> &m, vec3_base<T> v);
    template<typename T>
    static constexpr vec3_base<T> diag(
        const mat3_base<T> &m, std::size_t i);
    template<typename T>
    static constexpr vec3_base<T> inv_diag(
        const mat3_base<T> &m, std::size_t i);
    template<typename T>
    static constexpr mat3_base<T> minor_matrix(
        const mat4_base<T> &m, std::size_t i, std::size_t j);
    template<typename T>
    static constexpr T minor(
        const mat4_base<T> &m, std::size_t i, std::size_t j);
    template<typename T>
    static constexpr T determinant(const mat3_base<T> &m);
    template<typename T>
    static constexpr T determinant(const mat4_base<T> &m);
    template<typename T>
    static constexpr T cofactor(
        const mat4_base<T> &m, std::size_t i, std::size_t j);
    template<typename T>
    static constexpr mat4_base<T> adjugate(const mat4_base<T> &m);
    template<typename T>
    static constexpr mat4_base<T> inverse(const mat4_base<T> &m);
    template<typename T>
    static constexpr mat3_base<T> transpose(const mat3_base<T> &m);
    template<typename T>
    static constexpr mat4_base<T> transpose(const mat4_base<T> &m);
    template<typename T>
    static constexpr mat4_base<T> translate(
        const mat4_base<T> &m, vec3_base<T> v);
    template<typename T>
    static constexpr mat4_base<T> scale(
        const mat4_base<T> &m, vec3_base<T> v);
    // Rotation
    template<typename T>
    static constexpr vec2_base<T> rotate(vec2_base<T> v, T sin, T cos);
    template<typename T>
    static constexpr vec2_base<T> rotate(vec2_base<T> v, T angle);
    template<typename T>
    static constexpr vec3_base<T> rotate(
        vec3_base<T> v, T sin, T cos, vec3_base<T> n);
    template<typename T>
    static constexpr vec3_base<T> rotate(
        vec3_base<T> v, T angle, vec3_base<T> n);
    template<typename T>
    static constexpr vec3_base<T> rotate_x(vec3_base<T> v, T sin, T cos);
    template<typename T>
    static constexpr vec3_base<T> rotate_y(vec3_base<T> v, T sin, T cos);
    template<typename T>
    static constexpr vec3_base<T> rotate_z(vec3_base<T> v, T sin, T cos);
    template<typename T>
    static constexpr vec3_base<T> rotate_x(vec3_base<T> v, T angle);
    template<typename T>
    static constexpr vec3_base<T> rotate_y(vec3_base<T> v, T angle);
    template<typename T>
    static constexpr vec3_base<T> rotate_z(vec3_base<T> v, T angle);
    template<typename T>
    static constexpr mat4_base<T> rotate(
        const mat4_base<T> &m, T angle, vec3_base<T> v);
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
        vec3_base<T> eye, vec3_base<T> center, vec3_base<T> up);
    // Etc.
    static void gaussian_filter(
        std::size_t size, float std_dev, std::span<float> s);
    static void gaussian_filter(
        std::size_t xsize, std::size_t ysize, float std_dev,
        std::span<float> s);
    static bool is_aligned(void *p, std::size_t a);
    static void *align_ptr(void *p, std::size_t a);
    template<typename T>
    static T *align_ptr(void *p);
    auto *rnd_generator(void);
    void init(void);
    void seed_rand(rand_seed_t s);
    void rand_mat(std::span<float> m);
};

template<typename T> inline constexpr T Math::sq2(void) {
    return static_cast<T>(1.41421356237309514547462185873882845);
}

template<typename T> inline constexpr T Math::sq2_2(void) {
    return static_cast<T>(0.70710678118654757273731092936941423);
}

template<typename T> inline constexpr T Math::e(void) {
    return static_cast<T>(2.7182818284590450907955982984276488);
}

template<typename T> inline constexpr T Math::pi(void) {
    return static_cast<T>(3.14159265358979323846264338327950288);
}

template<typename T> inline constexpr T Math::tau(void) {
    return static_cast<T>(6.28318530717958647692528676655900576);
}

template<typename T> inline constexpr T Math::radians(T d) {
    constexpr auto mul = Math::tau<T>() / T{360};
    return d * mul;
}

template<typename T> inline constexpr T Math::degrees(T r) {
    constexpr auto mul = T{360} * Math::tau<T>();
    return r * mul;
}

template<typename T>
constexpr T Math::round_down_pow2(T n, T d) {
    assert(std::popcount(d) == 1);
    return n & ~--d;
}

template<typename T>
constexpr T Math::round_up_pow2(T n, T d) {
    assert(std::popcount(d) == 1);
    return --d, (n + d) & ~d;
}

template<typename T>
constexpr T Math::round_down(T n, T d) {
    if(const auto r = n % d)
        n -= r;
    return n;
}

template<typename T>
constexpr T Math::round_up(T n, T d) {
    if(const auto r = n % d)
        n += d - r;
    return n;
}

template<typename T>
constexpr T Math::round_up_div(T n, T d) {
    return (d - 1 + n) / d;
}

template<unsigned_integral T>
constexpr T Math::mip_levels(T extent) {
    assert(std::popcount(extent) == 1);
    return T{1} + static_cast<T>(std::countr_zero(std::bit_floor(extent)));
}

template<typename T>
inline constexpr auto Math::dot(T u, T v) {
    if constexpr(std::ranges::range<T>)
        return std::inner_product(
            std::ranges::begin(u), std::ranges::end(u), std::ranges::begin(v),
            std::ranges::range_value_t<T>{});
    else
        return u * v;
}

template<typename T>
inline constexpr auto Math::length2(T v) {
    return Math::dot(v, v);
}

template<typename T>
inline auto Math::length(T v) {
    return std::sqrt(Math::length2(v));
}

template<typename T>
inline T Math::normalize(T v) {
    return v / Math::length(v);
}

template<template<typename> typename V, typename T>
requires vector<V, T>
inline V<T> Math::clamp_len(V<T> v, T len) {
    const auto len2 = Math::length2(v), max2 = len * len;
    return len2 <= max2 ? v : v * std::sqrt(max2 / len2);
}

template<typename T>
inline constexpr vector_type<T> Math::sum(T v) {
    return std::accumulate(begin(v), end(v), vector_type<T>{});
}

template<typename T>
inline constexpr vector_type<T> Math::product(T v) {
    return std::accumulate(
        begin(v), end(v), vector_type<T>{1}, std::multiplies<>{});
}

template<typename T>
inline constexpr vector_type<T> Math::avg(T v) {
    return Math::sum(v) / vector_type<T>{T::n_dim};
}

template<typename T>
inline constexpr vec3_base<T> Math::cross(vec3_base<T> u, vec3_base<T> v) {
    return {
        u.y * v.z - u.z * v.y,
        u.z * v.x - u.x * v.z,
        u.x * v.y - u.y * v.x,
    };
}

template<typename T>
inline constexpr T Math::angle(vec2_base<T> u, vec2_base<T> v) {
    return Math::angle(vec3_base<T>(u), vec3_base<T>(v), {0, 0, 1});
}

template<typename T>
inline constexpr T Math::angle(vec3_base<T> u, vec3_base<T> v, vec3_base<T> n) {
    const auto cross = Math::cross(u, v);
    const auto ret = std::acos(Math::dot(u, v));
    return Math::dot(cross, n) < 0 ? Math::tau<T>() - ret : ret;
}

template<typename T>
inline constexpr vec3_base<T> Math::reflect(vec3_base<T> v, vec3_base<T> n) {
    return v - T(2) * n * Math::dot(v, n);
}

template<typename T>
inline constexpr vec3_base<T> Math::normal(
    vec3_base<T> p0, vec3_base<T> p1, vec3_base<T> p2)
{
    return Math::cross(p1 - p0, p2 - p0);
}

template<typename T>
inline constexpr mat3_base<T> Math::transpose(const mat3_base<T> &m) {
    return {m.row(0), m.row(1), m.row(2)};
}

template<typename T>
inline constexpr mat4_base<T> Math::transpose(const mat4_base<T> &m) {
    return {m.row(0), m.row(1), m.row(2), m.row(3)};
}

template<typename T>
inline vec3_base<T> Math::perspective_transform(
    const mat4_base<T> &m, vec3_base<T> v)
{
    const auto ret = m * vec4_base<T>{v, 1};
    return ret.xyz() / ret.w;
}

template<typename T>
inline constexpr vec3_base<T> Math::diag(
    const mat3_base<T> &m, std::size_t i)
{
    return {m[i][0], m[(i + 1) % 3][1], m[(i + 2) % 3][2]};
}

template<typename T>
inline constexpr vec3_base<T> Math::inv_diag(
    const mat3_base<T> &m, std::size_t i)
{
    return {m[i][0], m[(i + 2) % 3][1], m[(i + 1) % 3][2]};
}

template<typename T>
inline constexpr mat3_base<T> Math::minor_matrix(
    const mat4_base<T> &m, std::size_t i, std::size_t j)
{
    mat3_base<T> ret = {};
    std::size_t c = 0;
    for(; c != i; ++c) {
        std::size_t r = 0;
        for(   ; r != j; ++r) ret[c][r    ] = m[c][r];
        for(++r; r != 4; ++r) ret[c][r - 1] = m[c][r];
    }
    for(++c; c != 4; ++c) {
        std::size_t r = 0;
        for(   ; r != j; ++r) ret[c - 1][r    ] = m[c][r];
        for(++r; r != 4; ++r) ret[c - 1][r - 1] = m[c][r];
    }
    return ret;
}

template<typename T>
inline constexpr T Math::minor(
    const mat4_base<T> &m, std::size_t i, std::size_t j)
{
    return Math::determinant(Math::minor_matrix(m, i, j));
}

template<typename T>
inline constexpr T Math::determinant(const mat3_base<T> &m) {
    using M = Math;
    return M::product(M::diag(m, 0)) - M::product(M::inv_diag(m, 0))
        +  M::product(M::diag(m, 1)) - M::product(M::inv_diag(m, 1))
        +  M::product(M::diag(m, 2)) - M::product(M::inv_diag(m, 2));
}

template<typename T>
inline constexpr T Math::determinant(const mat4_base<T> &m) {
    using M = Math;
    return m[0][0] * M::minor(m, 0, 0)
        -  m[0][1] * M::minor(m, 0, 1)
        +  m[0][2] * M::minor(m, 0, 2)
        -  m[0][3] * M::minor(m, 0, 3);
}

template<typename T>
inline constexpr mat4_base<T> Math::adjugate(const mat4_base<T> &m) {
    mat4_base<T> ret = {};
    float s = 1.0f;
    for(std::size_t c = 0; c != 4; ++c, s = -s)
        for(std::size_t r = 0; r != 4; ++r, s = -s)
            ret[c][r] = s * Math::minor(m, r, c);
    return ret;
}

template<typename T>
inline constexpr mat4_base<T> Math::inverse(const mat4_base<T> &m) {
    return T{1} / Math::determinant(m) * Math::adjugate(m);
}

template<typename T>
inline constexpr mat4_base<T> Math::translate(
    const mat4_base<T> &m, vec3_base<T> v)
{
    return {
        m[0], m[1], m[2],
        {m[0] * v[0] + m[1] * v[1] + m[2] * v[2] + m[3]}};
}

template<typename T>
inline constexpr mat4_base<T> Math::scale(
    const mat4_base<T> &m, vec3_base<T> v)
{
    return {m[0] * v[0], m[1] * v[1], m[2] * v[2], m[3]};
}

template<typename T>
inline constexpr vec2_base<T> Math::rotate(vec2_base<T> v, T sin, T cos) {
    return {v.x * cos - v.y * sin, v.x * sin + v.y * cos};
}


template<typename T>
inline constexpr vec2_base<T> Math::rotate(vec2_base<T> v, T angle) {
    return Math::rotate(v, std::sin(angle), std::cos(angle));
}

template<typename T>
inline constexpr vec3_base<T> Math::rotate(
    vec3_base<T> v, T sin, T cos, vec3_base<T> n)
{
    const auto proj = n * Math::dot(n, v);
    const auto d = v - proj;
    const auto rot = cos * d + sin * Math::cross(n, d);
    return proj + rot;
}

template<typename T>
inline constexpr vec3_base<T> Math::rotate(
    vec3_base<T> v, T angle, vec3_base<T> n)
{
    return Math::rotate(v, std::sin(angle), std::cos(angle), n);
}

template<typename T>
inline constexpr vec3_base<T> Math::rotate_x(vec3_base<T> v, T sin, T cos) {
    const auto r = Math::rotate(v.yz(), sin, cos);
    return {v.x, r.x, r.y};
}

template<typename T>
inline constexpr vec3_base<T> Math::rotate_y(vec3_base<T> v, T sin, T cos) {
    const auto r = Math::rotate(v.zx(), sin, cos);
    return {r.y, v.y, r.x};
}

template<typename T>
inline constexpr vec3_base<T> Math::rotate_z(vec3_base<T> v, T sin, T cos) {
    const auto r = Math::rotate(v.xy(), sin, cos);
    return {r.x, r.y, v.z};
}

template<typename T>
inline constexpr vec3_base<T> Math::rotate_x(vec3_base<T> v, T angle) {
    return Math::rotate_x(v, std::sin(angle), std::cos(angle));
}

template<typename T>
inline constexpr vec3_base<T> Math::rotate_y(vec3_base<T> v, T angle) {
    return Math::rotate_y(v, std::sin(angle), std::cos(angle));
}

template<typename T>
inline constexpr vec3_base<T> Math::rotate_z(vec3_base<T> v, T angle) {
    return Math::rotate_z(v, std::sin(angle), std::cos(angle));
}

template<typename T>
inline constexpr mat4_base<T> Math::rotate(
    const mat4_base<T> &m, T angle, vec3_base<T> v)
{
    const T cos = std::cos(angle);
    const T sin = std::sin(angle);
    const auto axis = Math::normalize(v);
    const auto tmp = (T{1} - cos) * axis;
    mat4 rot = {};
    rot[0][0] = tmp.x * axis.x + cos;
    rot[0][1] = tmp.y * axis.x + sin * axis.z;
    rot[0][2] = tmp.z * axis.x - sin * axis.y;
    rot[1][0] = tmp.x * axis.y - sin * axis.z;
    rot[1][1] = tmp.y * axis.y + cos;
    rot[1][2] = tmp.z * axis.y + sin * axis.x;
    rot[2][0] = tmp.x * axis.z + sin * axis.y;
    rot[2][1] = tmp.y * axis.z - sin * axis.x;
    rot[2][2] = tmp.z * axis.z + cos;
    rot[3][3] = T{1};
    return m * rot;
}

template<typename T>
inline constexpr mat4_base<T> Math::ortho(T left, T right, T bottom, T top) {
    const auto w = right - left, h = top - bottom;
    auto ret = mat4_base<T>{0};
    ret[0][0] = T{2} / w;
    ret[1][1] = T{2} / h;
    ret[2][2] = T{-1};
    ret[3][3] = T{1};
    ret[3][0] = -(right + left) / w;
    ret[3][1] = -(top + bottom) / h;
    return ret;
}

template<typename T>
inline constexpr mat4_base<T> Math::ortho(
    T left, T right, T bottom, T top, T near, T far)
{
    auto ret = Math::ortho(left, right, bottom, top);
    const auto z = far - near;
    ret[2][2] = -T{2} / z;
    ret[3][2] = -(far + near) / z;
    return ret;
}

template<typename T>
inline constexpr mat4_base<T> Math::perspective(
    T fovy, T aspect, T near, T far)
{
    assert(std::abs(aspect) > -std::numeric_limits<T>::epsilon());
    const auto tan = std::tan(fovy / T{2});
    auto ret = mat4_base<T>{0};
    ret[0][0] = T{1} / (aspect * tan);
    ret[1][1] = T{1} / tan;
    ret[2][2] = -(far + near) / (far - near);
    ret[2][3] = -T{1};
    ret[3][2] = -T{2} * far * near / (far - near);
    return ret;
}

template<typename T>
inline constexpr mat4_base<T> Math::look_at(
    vec3_base<T> eye, vec3_base<T> center, vec3_base<T> up)
{
    const auto f = Math::normalize(center - eye);
    const auto s = Math::normalize(Math::cross(f, up));
    const auto u = Math::cross(s, f);
    return Math::transpose(mat4{
        vec4{s, -Math::dot(s, eye)},
        vec4{u, -Math::dot(u, eye)},
        vec4{-f, Math::dot(f, eye)},
        vec4{0, 0, 0, 1},
    });
}

inline bool Math::is_aligned(void *p, std::size_t a) {
    const auto u = reinterpret_cast<std::uintptr_t>(p);
    return !(u & --a);
}

inline void *Math::align_ptr(void *p, std::size_t a) {
    const auto u = reinterpret_cast<std::uintptr_t>(p);
    const auto r = Math::round_up_pow2(u, a);
    return reinterpret_cast<void*>(r);
}

template<typename T>
T *Math::align_ptr(void *p) {
    return static_cast<T*>(Math::align_ptr(p, alignof(T)));
}

inline auto *Math::rnd_generator(void) {
    assert(this->m_rnd_generator);
    return &*this->m_rnd_generator;
}

}

#endif
