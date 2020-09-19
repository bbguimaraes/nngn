#ifndef NNGN_RAY_SPHERE_H
#define NNGN_RAY_SPHERE_H

#include "math/math.h"

class Math;

namespace nngn::ray {

struct sphere {
    vec3 c = {};
    float r = {};
    material *mat = nullptr;
    explicit constexpr sphere(float c_r) : sphere({}, c_r) {}
    constexpr sphere(const vec3 &c_c, float c_r)
        : sphere(c_c, c_r, nullptr) {}
    constexpr sphere(const vec3 &c_c, float c_r, material *m)
        : c(c_c), r(c_r), mat(m) {}
    bool hit(const ray &ray, float t_min, float t_max, hit *h) const;
    vec3 random_point(Math::rnd_generator_t *rnd) const;
};

inline vec3 sphere::random_point(Math::rnd_generator_t *rnd) const {
    auto d = std::uniform_real_distribution<float>();
    return this->c + this->r * vec3(d(*rnd), d(*rnd), d(*rnd));
}

}

#endif
