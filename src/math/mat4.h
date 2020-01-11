#ifndef NNGN_MATH_MAT4_H
#define NNGN_MATH_MAT4_H

#include <array>
#include <type_traits>

#include "vec4.h"

namespace nngn {

template<typename T>
struct mat4_base {
    std::array<T, 16> m;
    constexpr mat4_base() = default;
    explicit constexpr mat4_base(T diag);
    constexpr mat4_base(vec4 col0, vec4 col1, vec4 col2, vec4 col3);
    constexpr mat4_base(
        T v0, T v1, T v2, T v3,
        T v4, T v5, T v6, T v7,
        T v8, T v9, T v10, T v11,
        T v12, T v13, T v14, T v15);
    constexpr vec4_base<T> &operator[](int i);
    constexpr const vec4_base<T> &operator[](int i) const;
    template<size_t I> constexpr std::enable_if_t<I < 4, vec4> col() const;
};

template<typename T>
inline constexpr mat4_base<T>::mat4_base(T diag)
    : m{
        diag, 0, 0, 0,
        0, diag, 0, 0,
        0, 0, diag, 0,
        0, 0, 0, diag} {}

template<typename T>
inline constexpr mat4_base<T>::mat4_base(
        vec4 col0, vec4 col1, vec4 col2, vec4 col3)
    : m{
        col0[0], col0[1], col0[2], col0[3],
        col1[0], col1[1], col1[2], col1[3],
        col2[0], col2[1], col2[2], col2[3],
        col3[0], col3[1], col3[2], col3[3]} {}

template<typename T>
inline constexpr mat4_base<T>::mat4_base(
        T v0, T v1, T v2, T v3,
        T v4, T v5, T v6, T v7,
        T v8, T v9, T v10, T v11,
        T v12, T v13, T v14, T v15)
    : m{
        v0, v1, v2, v3, v4, v5, v6, v7, v8,
        v9, v10, v11, v12, v13, v14, v15} {}

template<typename T>
inline constexpr vec4_base<T> &mat4_base<T>::operator[](int i) {
    return *static_cast<vec4_base<T>*>(
        static_cast<void*>(this->m.data() + i * 4));
}

template<typename T>
inline constexpr const vec4_base<T> &mat4_base<T>::operator[](int i) const {
    return *static_cast<const vec4_base<T>*>(
        static_cast<const void*>(this->m.data() + i * 4));
}

template<typename T>
template<size_t I>
inline constexpr std::enable_if_t<I < 4, vec4> mat4_base<T>::col() const
    { return {this->m[I], this->m[I + 4], this->m[I + 8], this->m[I + 12]}; }

template<typename T>
inline constexpr bool operator==(const mat4_base<T> &m0, const mat4_base<T> &m1)
    { return m0.m == m1.m; }

template<typename T>
inline constexpr mat4_base<T> operator*(
        const mat4_base<T> &m0, const mat4_base<T> &m1) {
    const auto
        c00 = m0[0], c01 = m0[1], c02 = m0[2], c03 = m0[3],
        c10 = m1[0], c11 = m1[1], c12 = m1[2], c13 = m1[3];
    return {
        c00 * c10[0] + c01 * c10[1] + c02 * c10[2] + c03 * c10[3],
        c00 * c11[0] + c01 * c11[1] + c02 * c11[2] + c03 * c11[3],
        c00 * c12[0] + c01 * c12[1] + c02 * c12[2] + c03 * c12[3],
        c00 * c13[0] + c01 * c13[1] + c02 * c13[2] + c03 * c13[3]};
}

template<typename T>
inline constexpr vec4_base<T> operator*(
        const mat4_base<T> &m, const vec4_base<T> &v) {
    const auto f = [](auto &&c) { return std::reduce(begin(c), end(c)); };
    return {f(v * m[0]), f(v * m[1]), f(v * m[2]), f(v * m[3])};
}

using imat4 = mat4_base<int32_t>;
using umat4 = mat4_base<uint32_t>;
using mat4 = mat4_base<float>;

}

#endif
