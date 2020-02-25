#ifndef NNGN_RAY_TRACER_H
#define NNGN_RAY_TRACER_H

#include <memory>
#include <vector>

#include "math/camera.h"
#include "math/math.h"
#include "utils/flags.h"

#include "dielectric.h"
#include "lambertian.h"
#include "metal.h"
#include "sphere.h"

namespace nngn::ray {

class world {
    std::vector<nngn::ray::sphere> spheres = {};
public:
    world(std::vector<nngn::ray::sphere> &&s) : spheres(std::move(s)) {}
    template<typename ...Args> sphere *add_sphere(Args &&...args);
    bool hit(const ray &r, float t_min, float t_max, hit *h) const;
};

class camera {
    vec3 o = {}, bl = {}, hor = {}, ver = {}, u = {}, v = {}, w = {};
    float fovy = {}, asp = {}, lens_radius = {}, focus_dist = {};
public:
    camera() = default;
    camera(float p_fovy, float p_asp, float aperture, float p_focus_dist) :
        fovy(p_fovy), asp(p_asp),
        lens_radius(aperture / 2), focus_dist(p_focus_dist) {}
    void set_pos(vec3 pos, const vec3 &eye, const vec3 &up);
    ray ray_uv(float u, float v, Math::rnd_generator_t *rnd) const;
};

class tracer {
    enum Flag {
        ENABLED,
    };
    Flags<Flag> flags = {};
    static constexpr size_t max_depth = 64;
    size_t img_width = 0, img_height = 0;
    size_t n_samples = 0, n_threads = 1;
    size_t max_samples = std::numeric_limits<size_t>::max();
    float min_t = 0, max_t = 0;
    std::vector<float> tex = {};
    std::vector<lambertian> lambertians = {};
    std::vector<metal> metals = {};
    std::vector<dielectric> dielectrics = {};
    world objects = {{}};
    Camera m_camera = {};
    nngn::ray::camera c = {};
    std::unique_ptr<struct thread_pool> pool;
    Math *math = nullptr;
    static constexpr vec3 interp(const vec3 &v, const vec3 &u, float t);
    vec3 color(ray r) const;
    vec3 read_pixel(size_t x, size_t y);
    void write_pixel(size_t x, size_t y, const vec4 &c);
public:
    tracer();
    tracer(tracer&) = delete;
    tracer &operator=(tracer&) = delete;
    ~tracer();
    Camera *camera() { return &this->m_camera; }
    void set_enabled(bool b) { this->flags.set(Flag::ENABLED, b); }
    void set_n_threads(size_t n) { this->n_threads = n; }
    void set_min_t(float t) { this->min_t = t; }
    void set_max_t(float t) { this->max_t = t; }
    void set_max_samples(size_t n) { this->max_samples = n; }
    void set_max_lambertians(size_t n);
    void set_max_metals(size_t n);
    void set_max_dielectrics(size_t n);
    void set_objects(world w) { this->objects = std::move(w); }
    void init(Math *m, size_t w, size_t h);
    void reset();
    template<typename ...Args> lambertian *add_lambertian(Args &&...a);
    template<typename ...Args> metal *add_metal(Args &&...a);
    template<typename ...Args> dielectric *add_dielectric(Args &&...a);
    template<typename ...Args> sphere *add_sphere(Args &&...a);
    bool trace(const Timing &t);
    void write_tex(size_t w, std::byte *p) const;
};

template<typename ...Args> sphere *world::add_sphere(Args &&...args)
    { return &this->spheres.emplace_back(std::forward<Args>(args)...); }
template<typename ...Args>
inline lambertian *tracer::add_lambertian(Args &&...a)
    { return &this->lambertians.emplace_back(std::forward<Args>(a)...); }
template<typename ...Args>
inline metal *tracer::add_metal(Args &&...a)
    { return &this->metals.emplace_back(std::forward<Args>(a)...); }
template<typename ...Args>
inline dielectric *tracer::add_dielectric(Args &&...a)
    { return &this->dielectrics.emplace_back(std::forward<Args>(a)...); }
template<typename ...Args>
inline sphere *tracer::add_sphere(Args &&...a)
    { return this->objects.add_sphere(std::forward<Args>(a)...); }

}

#endif
