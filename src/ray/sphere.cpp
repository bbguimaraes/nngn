#include "material.h"
#include "ray.h"
#include "sphere.h"

namespace nngn::ray {

bool sphere::hit(
        const ray &ray, float t_min, float t_max, nngn::ray::hit *h) const {
    const auto p = ray.p - this->c;
    const auto qa = Math::dot(ray.d, ray.d);
    const auto qb = Math::dot(p, ray.d);
    const auto qc = Math::dot(p, p) - this->r * this->r;
    const auto disc = qb * qb - qa * qc;
    if(disc < 0)
        return false;
    const auto f = [this, &ray, h, t_min, t_max](float t) {
        if(t < t_min || t_max < t)
            return false;
        const auto hp = ray.point_at(t);
        *h = nngn::ray::hit{t, hp, (hp - this->c) / this->r, this->mat};
        return true;
    };
    if(f((-qb - std::sqrt(disc)) / qa))
        return true;
    if(f((-qb + std::sqrt(disc)) / qa))
        return true;
    return false;
}

}
