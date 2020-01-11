#ifndef NNGN_MATH_VEC_H
#define NNGN_MATH_VEC_H

#include <cstddef>
#include <cstring>
#include <functional>
#include <numeric>
#include <utility>

#include "utils/concepts.h"
#include "utils/utils.h"

namespace nngn {

template<template<typename> typename CRTP, typename T, std::size_t N>
struct vec {
    using type = T;
    static constexpr std::size_t n_dim = N;
    template<typename R = T, typename F, typename ...Args>
    static constexpr CRTP<R> map(F f, Args &&...args);
    template<typename R = T, typename F, std::size_t ...I>
    static constexpr CRTP<R> map_impl(
        F f, std::index_sequence<I...>, const vec &v);
    template<typename R = T, typename F, std::size_t ...I>
    static constexpr CRTP<R> map_impl(
        F f, std::index_sequence<I...>, T t, const vec &v0);
    template<typename R = T, typename F, std::size_t ...I>
    static constexpr CRTP<R> map_impl(
        F f, std::index_sequence<I...>, const vec &v0, T t);
    template<typename R = T, typename F, std::size_t ...I>
    static constexpr CRTP<R> map_impl(
        F f, std::index_sequence<I...>, const vec &v0, const vec &v1);
    template<typename U> explicit constexpr operator CRTP<U>(void) const;
    constexpr vec &operator=(const CRTP<T> &v);
    constexpr T &operator[](int i);
    constexpr const T &operator[](int i) const;
    constexpr CRTP<T> operator-(void) const;
    constexpr CRTP<T> &operator+=(T t);
    constexpr CRTP<T> &operator-=(T t);
    constexpr CRTP<T> &operator*=(T t);
    constexpr CRTP<T> &operator/=(T t);
    constexpr CRTP<T> &operator+=(const CRTP<T> &v);
    constexpr CRTP<T> &operator-=(const CRTP<T> &v);
    constexpr CRTP<T> &operator*=(const CRTP<T> &v);
    constexpr CRTP<T> &operator/=(const CRTP<T> &v);
    template<std::size_t I> constexpr T &get(void);
    template<std::size_t I> constexpr const T &get(void) const;
    constexpr auto as_tuple(void) const;
};

template<template<typename> typename T, typename U>
concept vector = derived_from<T<U>, vec<T, U, T<U>::n_dim>>;

template<typename T> using vector_type = typename T::type;

template<template<typename> typename CRTP, typename T, std::size_t N>
template<typename R, typename F, typename ...Args>
inline constexpr CRTP<R> vec<CRTP, T, N>::map(F f, Args &&...args)
    { return vec::map_impl<R>(f, std::make_index_sequence<N>{}, FWD(args)...); }

template<template<typename> typename CRTP, typename T, std::size_t N>
template<typename R, typename F, std::size_t ...I>
inline constexpr CRTP<R> vec<CRTP, T, N>::map_impl(
        F f, std::index_sequence<I...>, const vec &v)
    { return {static_cast<R>(f(v.get<I>()))...}; }

template<template<typename> typename CRTP, typename T, std::size_t N>
template<typename R, typename F, std::size_t ...I>
inline constexpr CRTP<R> vec<CRTP, T, N>::map_impl(
        F f, std::index_sequence<I...>, T t, const vec &v)
    { return {static_cast<R>(f(t, v.get<I>()))...}; }

template<template<typename> typename CRTP, typename T, std::size_t N>
template<typename R, typename F, std::size_t ...I>
inline constexpr CRTP<R> vec<CRTP, T, N>::map_impl(
        F f, std::index_sequence<I...>, const vec &v, T t)
    { return {static_cast<R>(f(v.get<I>(), t))...}; }

template<template<typename> typename CRTP, typename T, std::size_t N>
template<typename R, typename F, std::size_t ...I>
inline constexpr CRTP<R> vec<CRTP, T, N>::map_impl(
        F f, std::index_sequence<I...>, const vec &v0, const vec &v1)
    { return {static_cast<R>(f(v0.get<I>(), v1.get<I>()))...}; }

template<template<typename> typename CRTP, typename T, std::size_t N>
template<typename U>
inline constexpr vec<CRTP, T, N>::operator CRTP<U>(void) const
    { return vec::map<U>([](auto x) { return static_cast<U>(x); }, *this); }

template<template<typename> typename CRTP, typename T, std::size_t N>
inline constexpr vec<CRTP, T, N> &vec<CRTP, T, N>::operator=(const CRTP<T> &v)
    { std::memcpy(this, &v, sizeof(CRTP<T>)); return *this; }

template<template<typename> typename CRTP, typename T, std::size_t N>
inline constexpr const T &vec<CRTP, T, N>::operator[](int i) const
    { return (&static_cast<const CRTP<T>*>(this)->x)[i]; }

template<template<typename> typename CRTP, typename T, std::size_t N>
inline constexpr T &vec<CRTP, T, N>::operator[](int i)
    { return const_cast<T&>(static_cast<const vec&>(*this)[i]); }

template<template<typename> typename CRTP, typename T, std::size_t N>
inline constexpr CRTP<T> vec<CRTP, T, N>::operator-(void) const
    { return vec::map(std::negate<>(), *this); }

template<template<typename> typename CRTP, typename T, std::size_t N>
inline constexpr CRTP<T> &vec<CRTP, T, N>::operator+=(T t)
    { auto &d = static_cast<CRTP<T>&>(*this); return d = d + t; }

template<template<typename> typename CRTP, typename T, std::size_t N>
inline constexpr CRTP<T> &vec<CRTP, T, N>::operator-=(T t)
    { auto &d = static_cast<CRTP<T>&>(*this); return d = d - t; }

template<template<typename> typename CRTP, typename T, std::size_t N>
inline constexpr CRTP<T> &vec<CRTP, T, N>::operator*=(T t)
    { auto &d = static_cast<CRTP<T>&>(*this); return d = d * t; }

template<template<typename> typename CRTP, typename T, std::size_t N>
inline constexpr CRTP<T> &vec<CRTP, T, N>::operator/=(T t)
    { auto &d = static_cast<CRTP<T>&>(*this); return d = d / t; }

template<template<typename> typename CRTP, typename T, std::size_t N>
inline constexpr CRTP<T> &vec<CRTP, T, N>::operator+=(const CRTP<T> &v)
    { auto &d = static_cast<CRTP<T>&>(*this); return d = d + v; }

template<template<typename> typename CRTP, typename T, std::size_t N>
inline constexpr CRTP<T> &vec<CRTP, T, N>::operator-=(const CRTP<T> &v)
    { auto &d = static_cast<CRTP<T>&>(*this); return d = d - v; }

template<template<typename> typename CRTP, typename T, std::size_t N>
inline constexpr CRTP<T> &vec<CRTP, T, N>::operator*=(const CRTP<T> &v)
    { auto &d = static_cast<CRTP<T>&>(*this); return d = d * v; }

template<template<typename> typename CRTP, typename T, std::size_t N>
inline constexpr CRTP<T> &vec<CRTP, T, N>::operator/=(const CRTP<T> &v)
    { auto &d = static_cast<CRTP<T>&>(*this); return d = d / v; }

template<template<typename> typename CRTP, typename T, std::size_t N>
template<std::size_t I> constexpr const T &vec<CRTP, T, N>::get(void) const {
    static_assert(I < N);
    auto *p = static_cast<const CRTP<T>*>(this);
    if constexpr(I == 0)
        return p->x;
    else if constexpr(I == 1)
        return p->y;
    else if constexpr(I == 2)
        return p->z;
    else if constexpr(I == 3)
        return p->w;
}

template<template<typename> typename CRTP, typename T, std::size_t N>
template<std::size_t I>
constexpr T &vec<CRTP, T, N>::get(void) {
    return const_cast<T&>(static_cast<const vec*>(this)->get<I>());
}

template<template<typename> typename CRTP, typename T, std::size_t N>
constexpr auto vec<CRTP, T, N>::as_tuple(void) const {
    return [this]<std::size_t ...I>(std::index_sequence<I...>) {
        return std::tuple{static_cast<const CRTP<T>&>(*this)[I]...};
    }(std::make_index_sequence<N>());
}

template<template<typename> typename T, typename R>
requires vector<T, R>
inline constexpr const R *begin(const T<R> &v) { return &v[0]; }

template<template<typename> typename T, typename R>
requires vector<T, R>
inline constexpr const R *end(const T<R> &v) { return &v[T<R>::n_dim]; }

template<template<typename> typename T, typename R>
requires vector<T, R>
inline constexpr R *begin(T<R> &v) { return &v[0]; }

template<template<typename> typename T, typename R>
requires vector<T, R>
inline constexpr R *end(T<R> &v) { return &v[T<R>::n_dim]; }

template<template<typename> typename T, typename R>
requires vector<T, R>
constexpr bool operator==(const T<R> &v0, const T<R> &v1)
    { return std::equal(begin(v0), end(v0), begin(v1)); }

template<template<typename> typename T, typename R>
requires vector<T, R>
constexpr bool operator!=(const T<R> &v0, const T<R> &v1)
    { return !(v0 == v1); }

template<template<typename> typename T, typename R>
requires vector<T, R>
inline constexpr T<R> operator+(R s, const T<R> &v)
    { return T<R>::map(std::plus<>(), s, v); }

template<template<typename> typename T, typename R>
requires vector<T, R>
inline constexpr T<R> operator-(R s, const T<R> &v)
    { return T<R>::map(std::minus<>(), s, v); }

template<template<typename> typename T, typename R>
requires vector<T, R>
inline constexpr T<R> operator*(R s, const T<R> &v)
    { return T<R>::map(std::multiplies<>(), s, v); }

template<template<typename> typename T, typename R>
requires vector<T, R>
inline constexpr T<R> operator/(R s, const T<R> &v)
    { return T<R>::map(std::divides<>(), s, v); }

template<template<typename> typename T, typename R>
requires vector<T, R>
inline constexpr T<R> operator+(const T<R> &v, R s)
    { return T<R>::map(std::plus<>(), v, s); }

template<template<typename> typename T, typename R>
requires vector<T, R>
inline constexpr T<R> operator-(const T<R> &v, R s)
    { return T<R>::map(std::minus<>(), v, s); }

template<template<typename> typename T, typename R>
requires vector<T, R>
inline constexpr T<R> operator*(const T<R> &v, R s)
    { return T<R>::map(std::multiplies<>(), v, s); }

template<template<typename> typename T, typename R>
requires vector<T, R>
inline constexpr T<R> operator/(const T<R> &v, R s)
    { return T<R>::map(std::divides<>(), v, s); }

template<template<typename> typename T, typename R>
requires vector<T, R>
inline constexpr T<R> operator+(const T<R> &v0, const T<R> &v1)
    { return T<R>::map(std::plus<>(), v0, v1); }

template<template<typename> typename T, typename R>
requires vector<T, R>
inline constexpr T<R> operator-(const T<R> &v0, const T<R> &v1)
    { return T<R>::map(std::minus<>(), v0, v1); }

template<template<typename> typename T, typename R>
requires vector<T, R>
inline constexpr T<R> operator*(const T<R> &v0, const T<R> &v1)
    { return T<R>::map(std::multiplies<>(), v0, v1); }

template<template<typename> typename T, typename R>
requires vector<T, R>
inline constexpr T<R> operator/(const T<R> &v0, const T<R> &v1)
    { return T<R>::map(std::divides<>(), v0, v1); }

}

#endif
