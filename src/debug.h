#ifndef NNGN_DEBUG_H
#define NNGN_DEBUG_H

#include <ostream>
#include <sstream>

#include "graphics/graphics.h"
#include "math/mat4.h"

namespace nngn {
    struct Vertex;
}

template<template<typename> typename V, typename T, size_t N>
std::ostream &operator <<(std::ostream &os, const nngn::vec<V, T, N> &v);
std::ostream &operator <<(std::ostream &os, const nngn::mat4 &m);
std::ostream &operator <<(std::ostream &os, const nngn::Vertex &v);

template<typename T0, typename T1>
inline std::string vdiff(const T0 &v0, const T1 &v1);

template<template<typename> typename V, typename T, size_t N>
inline std::ostream &operator <<(
        std::ostream &os, const nngn::vec<V, T, N> &v) {
    os << "{" << v[0];
    for(size_t i = 1; i < N; ++i)
        os << ", " << v[static_cast<int>(i)];
    return os << "}";
}

inline std::ostream &operator <<(std::ostream &os, const nngn::mat4 &m) {
    return os
        << "{"  << m.col<0>() << ", " << m.col<1>()
        << ", " << m.col<2>() << ", " << m.col<3>() << "}";
}

inline std::ostream &operator <<(std::ostream &os, const nngn::Vertex &v)
    { return os << "{"  << v.pos << ", " << v.color << "}"; }

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
