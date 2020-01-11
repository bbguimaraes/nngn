#ifndef NNGN_MATH_VEC3_H
#define NNGN_MATH_VEC3_H

#include <cstdint>

#include "vec.h"
#include "vec2.h"

namespace nngn {

template<typename T>
struct vec3_base : public vec<vec3_base, T, 3> {
    T x, y, z;
    constexpr vec3_base() = default;
    explicit constexpr vec3_base(T v) : vec3_base{v, v, v} {}
    constexpr vec3_base(T p_x, T p_y) : vec3_base{p_x, p_y, {}} {}
    constexpr vec3_base(T p_x, T p_y, T p_z) : x{p_x}, y{p_y}, z{p_z} {}
    explicit constexpr vec3_base(const vec2_base<T> &v)
        : vec3_base{v.x, v.y, {}} {}
    constexpr vec3_base(const vec2_base<T> &v, T p_z)
        : vec3_base{v.x, v.y, p_z} {}
    constexpr vec3_base(T p_x, const vec2_base<T> &v)
        : vec3_base{p_x, v[0], v[1]} {}
#define S(v0, v1) \
    constexpr vec2_base<T> v0 ## v1() const { return {this->v0, this->v1}; }
    S(x,x) S(x,y) S(x,z)
    S(y,x) S(y,y) S(y,z)
    S(z,x) S(z,y) S(z,z)
#undef S
#define S(v0, v1, v2) \
    constexpr vec3_base<T> v0 ## v1 ## v2() const \
        { return {this->v0, this->v1, this->v2}; }
    S(x,x,x) S(x,x,y) S(x,x,z)
    S(x,y,x) S(x,y,y) S(x,y,z)
    S(x,z,x) S(x,z,y) S(x,z,z)
    S(y,x,x) S(y,x,y) S(y,x,z)
    S(y,y,x) S(y,y,y) S(y,y,z)
    S(y,z,x) S(y,z,y) S(y,z,z)
    S(z,x,x) S(z,x,y) S(z,x,z)
    S(z,y,x) S(z,y,y) S(z,y,z)
    S(z,z,x) S(z,z,y) S(z,z,z)
#undef S
};

using ivec3 = vec3_base<std::int32_t>;
using uvec3 = vec3_base<std::uint32_t>;
using zvec3 = vec3_base<std::size_t>;
using vec3  = vec3_base<float>;
using dvec3 = vec3_base<double>;

static_assert(offsetof(ivec3, y) == sizeof(ivec3::type));
static_assert(offsetof(uvec3, y) == sizeof(uvec3::type));
static_assert(offsetof( vec3, y) == sizeof(vec3::type));
static_assert(offsetof(dvec3, y) == sizeof(dvec3::type));
static_assert(offsetof(ivec3, z) == sizeof(ivec3::type) * 2);
static_assert(offsetof(uvec3, z) == sizeof(uvec3::type) * 2);
static_assert(offsetof( vec3, z) == sizeof( vec3::type) * 2);
static_assert(offsetof(dvec3, z) == sizeof(dvec3::type) * 2);

}

#endif
