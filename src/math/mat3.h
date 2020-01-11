#ifndef NNGN_MATH_MAT3_H
#define NNGN_MATH_MAT3_H

#include <type_traits>

#include "mat.h"
#include "vec3.h"

namespace nngn {

template<typename T>
struct mat3_base : mat<mat3_base, T, 3> {
    constexpr mat3_base(void) = default;
    explicit constexpr mat3_base(T diag);
    constexpr mat3_base(vec3 col0, vec3 col1, vec3 col2);
    constexpr mat3_base(
        T v0, T v1, T v2,
        T v3, T v4, T v5,
        T v6, T v7, T v8);
};

template<typename T>
inline constexpr mat3_base<T>::mat3_base(T diag) {
    this->m = {
        diag, 0, 0,
        0, diag, 0,
        0, 0, diag,
    };
}

template<typename T>
inline constexpr mat3_base<T>::mat3_base(vec3 col0, vec3 col1, vec3 col2) {
    this->m = {
        col0[0], col0[1], col0[2],
        col1[0], col1[1], col1[2],
        col2[0], col2[1], col2[2],
    };
}

template<typename T>
inline constexpr mat3_base<T>::mat3_base(
    T v0, T v1, T v2,
    T v3, T v4, T v5,
    T v6, T v7, T v8)
{
    this->m = {v0, v1, v2, v3, v4, v5, v6, v7, v8};
}

using imat3 = mat3_base<int32_t>;
using umat3 = mat3_base<uint32_t>;
using mat3 = mat3_base<float>;
using dmat3 = mat3_base<double>;

}

#endif
