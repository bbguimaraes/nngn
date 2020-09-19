#include "metal.h"
#include "ray.h"
#include "sphere.h"

namespace nngn::ray {

std::tuple<bool, ray, vec3> metal::scatter(
        const ray &r, const hit &h, Math::rnd_generator_t *rnd) const {
    const auto refl =
        Math::reflect(Math::normalize(r.d), h.n)
        + this->fuzz * sphere(1).random_point(rnd);
    if(Math::dot(refl, h.n) <= 0)
        return {false, {}, {}};
    return {true, ray{h.p, refl}, this->albedo};
}

}
