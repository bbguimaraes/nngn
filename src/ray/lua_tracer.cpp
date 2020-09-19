#include <sol/sol.hpp>

#include "lua/state.h"
#include "math/camera.h"
#include "timing/timing.h"

#include "tracer.h"

using tracer = nngn::ray::tracer;

namespace {

auto add_lambertian(tracer &t, const sol::stack_table &a) {
    return static_cast<nngn::ray::material*>(
        t.add_lambertian(nngn::vec3(a[1], a[2], a[3])));
}

auto add_metal(tracer &t, const sol::stack_table &a, float f) {
    return static_cast<nngn::ray::material*>(
        t.add_metal(nngn::vec3(a[1], a[2], a[3]), f));
}

auto add_dielectric(tracer &t, float n) {
    return static_cast<nngn::ray::material*>(t.add_dielectric(n));
}

auto add_sphere(
    tracer &t, nngn::lua::table_view c, float r, nngn::ray::material *m
) {
    return t.add_sphere(nngn::vec3(c[1], c[2], c[3]), r, m);
}

void write_tex(
    tracer &t, std::vector<std::byte> *v, std::optional<bool> gamma_correction
) {
    t.write_tex(*v, gamma_correction.value_or(false));
}

}

NNGN_LUA_PROXY(tracer,
    "init", &tracer::init,
    "done", &tracer::done,
    "camera", &tracer::camera,
    "set_enabled", &tracer::set_enabled,
    "set_size", &tracer::set_size,
    "set_max_depth", &tracer::set_max_depth,
    "set_max_samples", &tracer::set_max_samples,
    "set_n_threads", &tracer::set_n_threads,
    "set_min_t", &tracer::set_min_t,
    "set_max_t", &tracer::set_max_t,
    "set_max_lambertians", &tracer::set_max_lambertians,
    "set_max_metals", &tracer::set_max_metals,
    "set_max_dielectrics", &tracer::set_max_dielectrics,
    "set_camera", &tracer::set_camera,
    "set_camera_aperture", &tracer::set_camera_aperture,
    "add_lambertian", add_lambertian,
    "add_metal", add_metal,
    "add_dielectric", add_dielectric,
    // XXX
    "add_sphere", add_sphere,
    "update", &tracer::update,
    "write_tex", write_tex)
