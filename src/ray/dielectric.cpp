#include "dielectric.h"
#include "ray.h"

namespace nngn::ray {

float dielectric::schlick(float cos) const {
    float r0 = (1 - this->n) / (1 + this->n);
    r0 *= r0;
    r0 += (1 - r0) * std::pow(1 - cos, 5.0f);
    return r0;
}

bool dielectric::refract(
        const vec3 &v, const vec3 &n,
        float n0, float n1, vec3 *ret) {
    const auto nv = Math::normalize(v);
    const auto div = n1 / n0;
    const auto dot = Math::dot(nv, n);
    const auto disc = 1 - div * div * (1 - dot * dot);
    if(disc <= 0)
        return false;
    *ret = div * (nv - n * dot) - n * std::sqrt(disc);
    return true;
}

std::tuple<bool, ray, vec3> dielectric::scatter(
        const ray &r, const hit &h, Math::rnd_generator_t *rnd) const {
    float n0 = 1, n1 = 1, cos = 0;
    vec3 out_n = h.n;
    const auto dot = Math::dot(r.d, h.n);
    if(dot > 0) {
        n1 = this->n;
        cos = this->n * dot / Math::length(r.d);
        out_n = -out_n;
    } else {
        n0 = this->n;
        cos = -dot / Math::length(r.d);
    }
    vec3 refr = {};
    float refr_p = 1;
    if(dielectric::refract(r.d, out_n, n0, n1, &refr))
        refr_p = this->schlick(cos);
    if(std::uniform_real_distribution<float>{}(*rnd) < refr_p)
        return {true, {h.p, Math::reflect(r.d, h.n)}, {1, 1, 1}};
    else
        return {true, {h.p, refr}, {1, 1, 1}};
}

}
