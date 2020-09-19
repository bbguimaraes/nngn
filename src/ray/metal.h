#ifndef NNGN_RAY_METAL_H
#define NNGN_RAY_METAL_H

#include "material.h"

namespace nngn::ray {

class metal : public material {
    vec3 albedo = {};
    float fuzz = 0;
public:
    metal(vec3 &&a, float f)
        : albedo(std::move(a)), fuzz(std::min(f, 1.0f)) {}
    std::tuple<bool, ray, vec3> scatter(
        const ray &r, const hit &h, Math::rnd_generator_t *rnd) const override;
};

}

#endif
