#ifndef NNGN_MATH_MAT_H
#define NNGN_MATH_MAT_H

#include <array>

#include "vec.h"

namespace nngn {

template<template<typename> typename CRTP, typename T, std::size_t N>
struct mat {
    using type = T;
    using vec_type = vec_type_t<T, N>;
    static constexpr std::size_t n_dim = N;
    constexpr vec_type &operator[](std::size_t i);
    constexpr const vec_type &operator[](std::size_t i) const;
    T *data(void) { return this->m.data(); }
    const T *data(void) const { return this->m.data(); }
    constexpr vec_type row(std::size_t i) const;
    constexpr vec_type col(std::size_t i) const;
    std::array<T, N * N> m = {};
};

template<template<typename> typename CRTP, typename T, std::size_t N>
constexpr auto mat<CRTP, T, N>::operator[](std::size_t i)
    -> vec_type&
{
    return *static_cast<vec_type*>(
        static_cast<void*>(this->data() + i * mat::n_dim));
}

template<template<typename> typename CRTP, typename T, std::size_t N>
constexpr auto mat<CRTP, T, N>::operator[](std::size_t i) const
    -> const vec_type&
{
    return *static_cast<const vec_type*>(
        static_cast<const void*>(this->data() + i * mat::n_dim));
}

template<template<typename> typename CRTP, typename T, std::size_t N>
constexpr auto mat<CRTP, T, N>::row(std::size_t i) const -> vec_type {
    vec_type ret = {};
    for(std::size_t c = 0; c != N; ++c)
        ret[c] = this->m[N * c + i];
    return ret;
}

template<template<typename> typename CRTP, typename T, std::size_t N>
constexpr auto mat<CRTP, T, N>::col(std::size_t i) const -> vec_type {
    vec_type ret = {};
    for(std::size_t r = 0; r != N; ++r)
        ret[r] = this->m[N * i + r];
    return ret;
}

template<template<typename> typename CRTP, typename T, std::size_t N>
constexpr bool operator==(
    const mat<CRTP, T, N> &m0, const mat<CRTP, T, N> &m1)
{
    return static_cast<const CRTP<T>&>(m0).m
        == static_cast<const CRTP<T>&>(m1).m;
}

template<template<typename> typename CRTP, typename T, std::size_t N>
inline constexpr CRTP<T> operator*(T s, const mat<CRTP, T, N> &m) {
    CRTP<T> ret = {};
    for(std::size_t i = 0; i != N; ++i)
        ret[i] = s * m[i];
    return ret;
}

template<template<typename> typename CRTP, typename T, std::size_t N>
inline constexpr CRTP<T> operator*(
    const mat<CRTP, T, N> &m0, const mat<CRTP, T, N> &m1)
{
    CRTP<T> ret = {};
    for(std::size_t c = 0; c != N; ++c)
        for(std::size_t r = 0; r != N; ++r)
            for(std::size_t i = 0; i != N; ++i)
                ret[c][r] += m0[i][r] * m1[c][i];
    return ret;
}

template<template<typename> typename CRTP, typename T, std::size_t N>
inline constexpr vec_type_t<T, N> operator*(
    const mat<CRTP, T, N> &m, const vec_type_t<T, N> &v)
{
    vec_type_t<T, N> ret = {};
    for(std::size_t i = 0; i != N; ++i) {
        const auto x = m.row(i) * v;
        ret[i] = std::reduce(begin(x), end(x));
    }
    return ret;
}

}

#endif
