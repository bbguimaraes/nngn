#ifndef NNGN_RAY_LAMBERTIAN_H
#define NNGN_RAY_LAMBERTIAN_H

#include "material.h"

namespace nngn::ray {

class lambertian : public material {
    vec3 albedo = {};
public:
    lambertian(vec3 a) : albedo(a) {}
    std::tuple<bool, ray, vec3> scatter(
        const ray &r, const hit &h, Math::rnd_generator_t *rnd) const override;
};

}

#endif
