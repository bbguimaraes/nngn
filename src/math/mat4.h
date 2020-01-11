#ifndef NNGN_MATH_MAT4_H
#define NNGN_MATH_MAT4_H

#include <type_traits>

#include "mat.h"
#include "vec4.h"

namespace nngn {

template<typename T>
struct mat4_base : mat<mat4_base, T, 4> {
    constexpr mat4_base(void) = default;
    explicit constexpr mat4_base(T diag);
    constexpr mat4_base(vec4 col0, vec4 col1, vec4 col2, vec4 col3);
    constexpr mat4_base(
        T  v0, T  v1, T  v2, T  v3,
        T  v4, T  v5, T  v6, T  v7,
        T  v8, T  v9, T v10, T v11,
        T v12, T v13, T v14, T v15);
};

template<typename T>
inline constexpr mat4_base<T>::mat4_base(T diag) {
    this->m = {
        diag, 0, 0, 0,
        0, diag, 0, 0,
        0, 0, diag, 0,
        0, 0, 0, diag,
    };
}

template<typename T>
inline constexpr mat4_base<T>::mat4_base(
    vec4 col0, vec4 col1, vec4 col2, vec4 col3)
{
    this->m = {
        col0[0], col0[1], col0[2], col0[3],
        col1[0], col1[1], col1[2], col1[3],
        col2[0], col2[1], col2[2], col2[3],
        col3[0], col3[1], col3[2], col3[3],
    };
}

template<typename T>
inline constexpr mat4_base<T>::mat4_base(
    T  v0, T  v1, T  v2, T  v3,
    T  v4, T  v5, T  v6, T  v7,
    T  v8, T  v9, T v10, T v11,
    T v12, T v13, T v14, T v15)
{
    this->m = {
         v0,  v1,  v2,  v3,
         v4,  v5,  v6,  v7,
         v8,  v9, v10, v11,
        v12, v13, v14, v15,
    };
}

using imat4 = mat4_base<int32_t>;
using umat4 = mat4_base<uint32_t>;
using mat4 = mat4_base<float>;
using dmat4 = mat4_base<double>;

}

#endif
