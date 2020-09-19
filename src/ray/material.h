#ifndef NNGN_RAY_MATERIAL_H
#define NNGN_RAY_MATERIAL_H

#include "math/math.h"

#include "ray.h"
#include "sphere.h"

namespace nngn::ray {

struct hit;
struct ray;

struct material {
    virtual ~material(void) {}
    virtual std::tuple<bool, ray, vec3> scatter(
        ray r, hit h, Math::rnd_generator_t *rnd) const = 0;
};

class lambertian : public material {
public:
    explicit lambertian(vec3 albedo_) : albedo(albedo_) {}
    std::tuple<bool, ray, vec3> scatter(
        ray r, hit h, Math::rnd_generator_t *rnd) const override;
private:
    vec3 albedo = {};
};

class metal : public material {
public:
    metal(vec3 albedo_, float fuzz_)
        : albedo{albedo_}, fuzz{std::min(fuzz_, 1.0f)} {}
    std::tuple<bool, ray, vec3> scatter(
        ray r, hit h, Math::rnd_generator_t *rnd) const override;
private:
    vec3 albedo = {};
    float fuzz = 0;
};

class dielectric : public material {
public:
    explicit dielectric(float c_n) : n(c_n) {}
    std::tuple<bool, ray, vec3> scatter(
        ray r, hit h, Math::rnd_generator_t *rnd) const override;
private:
    float n = {};
    static bool refract(vec3 v, vec3 n, float n0, float n1, vec3 *ret);
    float schlick(float cos) const;
};

inline std::tuple<bool, ray, vec3> lambertian::scatter(
    ray, hit h, Math::rnd_generator_t *rnd
) const {
    return {true, {h.p, h.n + sphere(1).random_point(rnd)}, this->albedo};
}

inline std::tuple<bool, ray, vec3> metal::scatter(
    ray r, hit h, Math::rnd_generator_t *rnd
) const {
    const auto refl =
        Math::reflect(Math::normalize(r.d), h.n)
        + this->fuzz * sphere(1).random_point(rnd);
    if(Math::dot(refl, h.n) <= 0)
        return {false, {}, {}};
    return {true, ray{h.p, refl}, this->albedo};
}

inline float dielectric::schlick(float cos) const {
    float r0 = (1 - this->n) / (1 + this->n);
    r0 *= r0;
    r0 += (1 - r0) * std::pow(1 - cos, 5.0f);
    return r0;
}

inline bool dielectric::refract(vec3 v, vec3 n, float n0, float n1, vec3 *ret) {
    const auto nv = Math::normalize(v);
    const auto div = n1 / n0;
    const auto dot = Math::dot(nv, n);
    const auto disc = 1 - div * div * (1 - dot * dot);
    if(disc <= 0)
        return false;
    *ret = div * (nv - n * dot) - n * std::sqrt(disc);
    return true;
}

inline std::tuple<bool, ray, vec3> dielectric::scatter(
    ray r, hit h, Math::rnd_generator_t *rnd
) const {
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

#endif
