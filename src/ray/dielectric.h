#ifndef NNGN_RAY_DIELECTRIC_H
#define NNGN_RAY_DIELECTRIC_H

#include "material.h"

namespace nngn::ray {

class dielectric : public material {
    float n = {};
    static bool refract(
        const vec3 &v, const vec3 &n, float n0, float n1, vec3 *ret);
    float schlick(float cos) const;
public:
    dielectric(float c_n) : n(c_n) {}
    std::tuple<bool, ray, vec3> scatter(
        const ray &r, const hit &h, Math::rnd_generator_t *rnd) const override;
};

}

#endif
