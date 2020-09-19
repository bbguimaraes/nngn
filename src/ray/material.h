#ifndef NNGN_RAY_MATERIAL_H
#define NNGN_RAY_MATERIAL_H

#include "math/math.h"

namespace nngn::ray {

struct hit;
struct ray;

struct material {
    virtual ~material() {}
    virtual std::tuple<bool, ray, vec3> scatter(
        const ray &r, const hit &h, Math::rnd_generator_t *rnd) const = 0;
};

}

#endif
