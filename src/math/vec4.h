#ifndef NNGN_MATH_VEC4_H
#define NNGN_MATH_VEC4_H

#include <cstdint>

#include "vec.h"
#include "vec3.h"

namespace nngn {

template<typename T>
struct vec4_base : public vec<vec4_base, T, 4> {
    T x, y, z, w;
    constexpr vec4_base() = default;
    explicit constexpr vec4_base(T v) : vec4_base{v, v, v, v} {}
    constexpr vec4_base(T p_x, T p_y)
        : vec4_base{p_x, p_y, {}, {}} {}
    constexpr vec4_base(T p_x, T p_y, T p_z)
        : vec4_base{p_x, p_y, p_z, {}} {}
    constexpr vec4_base(T p_x, T p_y, T p_z, T p_w)
        : x{p_x}, y{p_y}, z{p_z}, w{p_w} {}
    constexpr vec4_base(const vec2_base<T> &v, T p_z, T p_w)
        : vec4_base{v.x, v.y, p_z, p_w} {}
    constexpr vec4_base(T p_x, const vec2_base<T> &v, T p_w)
        : vec4_base{p_x, v.x, v.y, p_w} {}
    constexpr vec4_base(const vec2_base<T> &u, const vec2_base<T> &v)
        : vec4_base{u.x, u.y, v.x, v.y} {}
    constexpr vec4_base(const vec3_base<T> &v, T p_w)
        : vec4_base{v.x, v.y, v.z, p_w} {}
    constexpr vec4_base(T p_x, const vec3_base<T> &v)
        : vec4_base{p_x, v.x, v.y, v.z} {}
    constexpr vec3_base<T> persp_div() const { return this->xyz() / this->w; }
#define S(v0, v1) \
    constexpr vec2_base<T> v0 ## v1() const { return {this->v0, this->v1}; }
    S(x,x) S(x,y) S(x,z) S(x,w)
    S(y,x) S(y,y) S(y,z) S(y,w)
    S(z,x) S(z,y) S(z,z) S(z,w)
    S(w,x) S(w,y) S(w,z) S(w,w)
#undef S
#define S(v0, v1, v2) \
    constexpr vec3_base<T> v0 ## v1 ## v2() const \
        { return {this->v0, this->v1, this->v2}; }
    S(x,x,x) S(x,x,y) S(x,x,z) S(x,x,w) S(x,y,x) S(x,y,y) S(x,y,z) S(x,y,w)
    S(x,z,x) S(x,z,y) S(x,z,z) S(x,z,w) S(x,w,x) S(x,w,y) S(x,w,z) S(x,w,w)
    S(y,x,x) S(y,x,y) S(y,x,z) S(y,x,w) S(y,y,x) S(y,y,y) S(y,y,z) S(y,y,w)
    S(y,z,x) S(y,z,y) S(y,z,z) S(y,z,w) S(y,w,x) S(y,w,y) S(y,w,z) S(y,w,w)
    S(z,x,x) S(z,x,y) S(z,x,z) S(z,x,w) S(z,y,x) S(z,y,y) S(z,y,z) S(z,y,w)
    S(z,z,x) S(z,z,y) S(z,z,z) S(z,z,w) S(z,w,x) S(z,w,y) S(z,w,z) S(z,w,w)
    S(w,x,x) S(w,x,y) S(w,x,z) S(w,x,w) S(w,y,x) S(w,y,y) S(w,y,z) S(w,y,w)
    S(w,z,x) S(w,z,y) S(w,z,z) S(w,z,w) S(w,w,x) S(w,w,y) S(w,w,z) S(w,w,w)
#undef S
#define S(v0, v1, v2, v3) \
    constexpr vec4_base<T> v0 ## v1 ## v2 ## v3() const \
        { return {this->v0, this->v1, this->v2, this->v3}; }
    S(x,x,x,x) S(x,x,x,y) S(x,x,x,z) S(x,x,x,w)
    S(x,x,y,x) S(x,x,y,y) S(x,x,y,z) S(x,x,y,w)
    S(x,x,z,x) S(x,x,z,y) S(x,x,z,z) S(x,x,z,w)
    S(x,x,w,x) S(x,x,w,y) S(x,x,w,z) S(x,x,w,w)
    S(x,y,x,x) S(x,y,x,y) S(x,y,x,z) S(x,y,x,w)
    S(x,y,y,x) S(x,y,y,y) S(x,y,y,z) S(x,y,y,w)
    S(x,y,z,x) S(x,y,z,y) S(x,y,z,z) S(x,y,z,w)
    S(x,y,w,x) S(x,y,w,y) S(x,y,w,z) S(x,y,w,w)
    S(x,z,x,x) S(x,z,x,y) S(x,z,x,z) S(x,z,x,w)
    S(x,z,y,x) S(x,z,y,y) S(x,z,y,z) S(x,z,y,w)
    S(x,z,z,x) S(x,z,z,y) S(x,z,z,z) S(x,z,z,w)
    S(x,z,w,x) S(x,z,w,y) S(x,z,w,z) S(x,z,w,w)
    S(x,w,x,x) S(x,w,x,y) S(x,w,x,z) S(x,w,x,w)
    S(x,w,y,x) S(x,w,y,y) S(x,w,y,z) S(x,w,y,w)
    S(x,w,z,x) S(x,w,z,y) S(x,w,z,z) S(x,w,z,w)
    S(x,w,w,x) S(x,w,w,y) S(x,w,w,z) S(x,w,w,w)
    S(y,x,x,x) S(y,x,x,y) S(y,x,x,z) S(y,x,x,w)
    S(y,x,y,x) S(y,x,y,y) S(y,x,y,z) S(y,x,y,w)
    S(y,x,z,x) S(y,x,z,y) S(y,x,z,z) S(y,x,z,w)
    S(y,x,w,x) S(y,x,w,y) S(y,x,w,z) S(y,x,w,w)
    S(y,y,x,x) S(y,y,x,y) S(y,y,x,z) S(y,y,x,w)
    S(y,y,y,x) S(y,y,y,y) S(y,y,y,z) S(y,y,y,w)
    S(y,y,z,x) S(y,y,z,y) S(y,y,z,z) S(y,y,z,w)
    S(y,y,w,x) S(y,y,w,y) S(y,y,w,z) S(y,y,w,w)
    S(y,z,x,x) S(y,z,x,y) S(y,z,x,z) S(y,z,x,w)
    S(y,z,y,x) S(y,z,y,y) S(y,z,y,z) S(y,z,y,w)
    S(y,z,z,x) S(y,z,z,y) S(y,z,z,z) S(y,z,z,w)
    S(y,z,w,x) S(y,z,w,y) S(y,z,w,z) S(y,z,w,w)
    S(y,w,x,x) S(y,w,x,y) S(y,w,x,z) S(y,w,x,w)
    S(y,w,y,x) S(y,w,y,y) S(y,w,y,z) S(y,w,y,w)
    S(y,w,z,x) S(y,w,z,y) S(y,w,z,z) S(y,w,z,w)
    S(y,w,w,x) S(y,w,w,y) S(y,w,w,z) S(y,w,w,w)
    S(z,x,x,x) S(z,x,x,y) S(z,x,x,z) S(z,x,x,w)
    S(z,x,y,x) S(z,x,y,y) S(z,x,y,z) S(z,x,y,w)
    S(z,x,z,x) S(z,x,z,y) S(z,x,z,z) S(z,x,z,w)
    S(z,x,w,x) S(z,x,w,y) S(z,x,w,z) S(z,x,w,w)
    S(z,y,x,x) S(z,y,x,y) S(z,y,x,z) S(z,y,x,w)
    S(z,y,y,x) S(z,y,y,y) S(z,y,y,z) S(z,y,y,w)
    S(z,y,z,x) S(z,y,z,y) S(z,y,z,z) S(z,y,z,w)
    S(z,y,w,x) S(z,y,w,y) S(z,y,w,z) S(z,y,w,w)
    S(z,z,x,x) S(z,z,x,y) S(z,z,x,z) S(z,z,x,w)
    S(z,z,y,x) S(z,z,y,y) S(z,z,y,z) S(z,z,y,w)
    S(z,z,z,x) S(z,z,z,y) S(z,z,z,z) S(z,z,z,w)
    S(z,z,w,x) S(z,z,w,y) S(z,z,w,z) S(z,z,w,w)
    S(z,w,x,x) S(z,w,x,y) S(z,w,x,z) S(z,w,x,w)
    S(z,w,y,x) S(z,w,y,y) S(z,w,y,z) S(z,w,y,w)
    S(z,w,z,x) S(z,w,z,y) S(z,w,z,z) S(z,w,z,w)
    S(z,w,w,x) S(z,w,w,y) S(z,w,w,z) S(z,w,w,w)
    S(w,x,x,x) S(w,x,x,y) S(w,x,x,z) S(w,x,x,w)
    S(w,x,y,x) S(w,x,y,y) S(w,x,y,z) S(w,x,y,w)
    S(w,x,z,x) S(w,x,z,y) S(w,x,z,z) S(w,x,z,w)
    S(w,x,w,x) S(w,x,w,y) S(w,x,w,z) S(w,x,w,w)
    S(w,y,x,x) S(w,y,x,y) S(w,y,x,z) S(w,y,x,w)
    S(w,y,y,x) S(w,y,y,y) S(w,y,y,z) S(w,y,y,w)
    S(w,y,z,x) S(w,y,z,y) S(w,y,z,z) S(w,y,z,w)
    S(w,y,w,x) S(w,y,w,y) S(w,y,w,z) S(w,y,w,w)
    S(w,z,x,x) S(w,z,x,y) S(w,z,x,z) S(w,z,x,w)
    S(w,z,y,x) S(w,z,y,y) S(w,z,y,z) S(w,z,y,w)
    S(w,z,z,x) S(w,z,z,y) S(w,z,z,z) S(w,z,z,w)
    S(w,z,w,x) S(w,z,w,y) S(w,z,w,z) S(w,z,w,w)
    S(w,w,x,x) S(w,w,x,y) S(w,w,x,z) S(w,w,x,w)
    S(w,w,y,x) S(w,w,y,y) S(w,w,y,z) S(w,w,y,w)
    S(w,w,z,x) S(w,w,z,y) S(w,w,z,z) S(w,w,z,w)
    S(w,w,w,x) S(w,w,w,y) S(w,w,w,z) S(w,w,w,w)
#undef S
};

template<typename T>
struct vec_type<T, 4> : std::type_identity<vec4_base<T>> {};

using ivec4 = vec4_base<std::int32_t>;
using uvec4 = vec4_base<std::uint32_t>;
using zvec4 = vec4_base<std::size_t>;
using vec4  = vec4_base<float>;
using dvec4 = vec4_base<double>;

static_assert(offsetof(ivec4, y) == sizeof(ivec4::type));
static_assert(offsetof(uvec4, y) == sizeof(uvec4::type));
static_assert(offsetof( vec4, y) == sizeof( vec4::type));
static_assert(offsetof(dvec4, y) == sizeof(dvec4::type));
static_assert(offsetof(ivec4, z) == sizeof(ivec4::type) * 2);
static_assert(offsetof(uvec4, z) == sizeof(uvec4::type) * 2);
static_assert(offsetof( vec4, z) == sizeof( vec4::type) * 2);
static_assert(offsetof(dvec4, z) == sizeof(dvec4::type) * 2);
static_assert(offsetof(ivec4, w) == sizeof(ivec4::type) * 3);
static_assert(offsetof(uvec4, w) == sizeof(uvec4::type) * 3);
static_assert(offsetof( vec4, w) == sizeof( vec4::type) * 3);
static_assert(offsetof(dvec4, w) == sizeof(dvec4::type) * 3);

}

#endif
