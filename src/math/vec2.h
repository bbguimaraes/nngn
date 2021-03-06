#ifndef NNGN_MATH_VEC2_H
#define NNGN_MATH_VEC2_H

#include <cstdint>

#include "vec.h"

namespace nngn {

template<typename T>
struct vec2_base : public vec<vec2_base, T, 2> {
    T x, y;
    constexpr vec2_base() = default;
    explicit constexpr vec2_base(T v) : x(v), y(v) {}
    constexpr vec2_base(T p_x, T p_y) : x(p_x), y(p_y) {}
#define S(v0, v1) \
    constexpr vec2_base<T> v0 ## v1() const { return {this->v0, this->v1}; }
    S(x,x) S(x,y)
    S(y,x) S(y,y)
#undef S
};

using ivec2 = vec2_base<int32_t>;
using uvec2 = vec2_base<uint32_t>;
using vec2 = vec2_base<float>;
using dvec2 = vec2_base<double>;

static_assert(offsetof(ivec2, y) == sizeof(ivec2::type));
static_assert(offsetof(uvec2, y) == sizeof(uvec2::type));
static_assert(offsetof( vec2, y) == sizeof( vec2::type));
static_assert(offsetof(dvec2, y) == sizeof(dvec2::type));

}

#endif
