#include "lambertian.h"
#include "ray.h"
#include "sphere.h"

namespace nngn::ray {

std::tuple<bool, ray, vec3> lambertian::scatter(
        const ray&, const hit &h, Math::rnd_generator_t *rnd) const
    { return {true, {h.p, h.n + sphere(1).random_point(rnd)}, this->albedo}; }

}
