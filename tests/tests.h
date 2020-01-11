#ifndef NNGN_TESTS_TESTS_H
#define NNGN_TESTS_TESTS_H

#include <sstream>
#include <string>

#include <QTest>

#include "debug.h"

#include "graphics/graphics.h"
#include "math/mat4.h"

template<template<typename> typename V, typename T, size_t N>
bool fuzzy_eq(const nngn::vec<V, T, N> &v0, const nngn::vec<V, T, N> &v1);

namespace std {

char *toString(const string &s);

char *toString(const string &s) { return QTest::toString(s.c_str()); }

}

namespace nngn {

template<typename T> char *toString(const vec2_base<T> &v);
template<typename T> char *toString(const vec3_base<T> &v);
template<typename T> char *toString(const vec4_base<T> &v);
char *toString(const mat4 &v);
char *toString(const Vertex &v);

}

template<template<typename> typename V, typename T, size_t N>
inline bool fuzzy_eq(
        const nngn::vec<V, T, N> &v0, const nngn::vec<V, T, N> &v1) {
    const auto f = [](T t0, T t1) {
        return t0 == 0 || t1 == 0
            ? qFuzzyCompare(t0 + 1, t1 + 1)
            : qFuzzyCompare(t0, t1);
    };
    for(int i = 0; i < static_cast<int>(N); ++i)
        if(!f(v0[i], v1[i]))
            return false;
    return true;
}

namespace nngn {

namespace {

template<typename T> inline char *to_string(const T &v) {
    std::stringstream s;
    s << v;
    return QTest::toString(s.str().c_str());
}

}

#define F(T) inline char *toString(const T &t) { return to_string(t); }
template<typename T> F(vec2_base<T>)
template<typename T> F(vec3_base<T>)
template<typename T> F(vec4_base<T>)
F(mat4)
#undef F

}

#endif
