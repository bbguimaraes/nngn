#ifndef NNGN_TESTS_TESTS_H
#define NNGN_TESTS_TESTS_H

#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <QTest>

#include "debug.h"

#include "graphics/graphics.h"
#include "utils/literals.h"

using namespace nngn::literals;
using nngn::u32;

namespace std {

inline char *toString(const string &s) { return QTest::toString(s.c_str()); }

inline char *toString(string_view s) {
    return QTest::toString(
        QString::fromUtf8(s.data(), static_cast<int>(s.size())));
}

template<typename T, typename A>
requires(requires(ostream &o, T t) { o << t; })
char *toString(const vector<T, A> &v) {
    stringstream s;
    s << v;
    return toString(s.str());
}

}

inline constexpr auto fuzzy_eq_e = 1_u32 << 4;

inline bool fuzzy_eq(float f0, float f1, u32 e = fuzzy_eq_e) {
    if(f0 == 0 || f1 == 0)
        ++f0, ++f1;
    return qFloatDistance(f0, f1) < e;
}

template<template<typename> typename V, typename T, std::size_t N>
inline bool fuzzy_eq(
    const nngn::vec<V, T, N> &v0,
    const nngn::vec<V, T, N> &v1,
    u32 e = fuzzy_eq_e)
{
    for(std::size_t i = 0; i < N; ++i)
        if(!fuzzy_eq(v0[i], v1[i], e))
            return false;
    return true;
}

template<typename T>
inline bool fuzzy_eq(
    const nngn::mat4_base<T> &m0,
    const nngn::mat4_base<T> &m1,
    u32 e = fuzzy_eq_e)
{
    for(std::size_t i = 0; i != 4; ++i)
        if(!fuzzy_eq(m0[i], m1[i], e))
            return false;
    return true;
}

namespace nngn {

namespace {

template<typename T>
inline char *to_string(const T &v) {
    std::stringstream s;
    s << v;
    return QTest::toString(s.str().c_str());
}

}

#define F(T) inline char *toString(const T &t) { return to_string(t); }
template<typename T> F(vec2_base<T>)
template<typename T> F(vec3_base<T>)
template<typename T> F(vec4_base<T>)
F(mat3)
F(mat4)
#undef F

}

#endif
