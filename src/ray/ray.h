#ifndef NNGN_RAY_RAY_H
#define NNGN_RAY_RAY_H

#include "math/vec3.h"

namespace nngn::ray {

struct material;

struct ray {
    vec3 p = {}, d = {};
    constexpr vec3 point_at(float t) const { return p + d * t; }
};

struct hit {
    float t = {};
    vec3 p = {}, n = {};
    material *mat = nullptr;
};

}

#endif
