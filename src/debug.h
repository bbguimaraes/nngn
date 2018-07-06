#ifndef NNGN_DEBUG_H
#define NNGN_DEBUG_H

#include <ostream>
#include <sstream>
#include <utility>
#include <vector>

#include "graphics/graphics.h"
#include "math/mat3.h"
#include "math/mat4.h"

namespace std {

template<typename T0, typename T1>
requires(requires (ostream &o, T0 t0, T1 t1) { o << t0; o << t1; })
ostream &operator<<(ostream &o, const pair<T0, T1> &p) {
    return o << '{' << p.first << ", " << p.second << '}';
}

template<typename T, size_t N>
requires(requires (ostream &o, T t) { o << t; })
ostream &operator<<(ostream &o, const array<T, N> &v) {
    if(v.empty())
        return o << "{}";
    o << '{' << v[0];
    for(auto b = cbegin(v) + 1, e = cend(v); b != e; ++b)
        o << ", " << *b;
    return o << '}';
}

template<typename T, typename A>
requires(requires (ostream &o, T t) { o << t; })
ostream &operator<<(ostream &o, const vector<T, A> &v) {
    if(v.empty())
        return o << "{}";
    o << '{' << v[0];
    for(auto b = cbegin(v) + 1, e = cend(v); b != e; ++b)
        o << ", " << *b;
    return o << '}';
}

}

namespace nngn {

struct Vertex;

template<template<typename> typename V, typename T, std::size_t N>
std::ostream &operator <<(std::ostream &os, const vec<V, T, N> &v);
std::ostream &operator <<(std::ostream &os, const mat4 &m);
std::ostream &operator <<(std::ostream &os, const Vertex &v);

template<template<typename> typename V, typename T, std::size_t N>
inline std::ostream &operator <<(std::ostream &os, const vec<V, T, N> &v) {
    os << "{" << v[0];
    for(std::size_t i = 1; i < N; ++i)
        os << ", " << v[i];
    return os << "}";
}

inline std::ostream &operator <<(std::ostream &os, const mat3 &m) {
    return os << "{" << m[0] << ", " << m[1] << ", " << m[2] << "}";
}

inline std::ostream &operator <<(std::ostream &os, const mat4 &m) {
    return os
        << "{"
        << m[0] << ", " << m[1] << ", "
        << m[2] << ", " << m[3]
        << "}";
}

inline std::ostream &operator <<(std::ostream &os, const Vertex &v) {
    return os << "{" << v.pos << ", " << v.color << "}";
}

}

template<typename T0, typename T1>
inline std::string vdiff(const T0 &v0, const T1 &v1);

template<typename T0, typename T1>
inline std::string vdiff(const T0 &v0, const T1 &v1) {
    using std::cbegin;
    using std::cend;
    const auto b0 = cbegin(v0), e0 = cend(v0);
    const auto e1 = cend(v1);
    auto p0 = b0;
    auto p1 = cbegin(v1);
    while(p0 != e0 && *p0 == *p1) { ++p0; ++p1; }
    if(p0 == e0 && p1 == e1)
        return "";
    const auto i = p0 - b0;
    std::stringstream s;
    const auto fmt = [&s, i](const char *n, auto p, const auto e) {
        s << "\n" << n << "[" << i << ":]:";
        while(p != e)
            s << " " << *p++;
    };
    fmt("v0", p0, e0);
    fmt("v1", p1, e1);
    s << '\n';
    return s.str();
}

#endif
