#ifndef NNGN_RAY_TRACER_H
#define NNGN_RAY_TRACER_H

#include <barrier>
#include <memory>
#include <vector>

#include "compute/thread_pool.h"
#include "math/camera.h"
#include "math/math.h"
#include "utils/def.h"
#include "utils/flags.h"

#include "material.h"
#include "sphere.h"

namespace nngn::ray {

class world {
public:
    template<typename ...Args> sphere *add_sphere(Args &&...args);
    bool hit(ray r, float t_min, float t_max, hit *h) const;
private:
    std::vector<nngn::ray::sphere> spheres = {};
};

class camera {
    vec3 o = {}, bl = {}, hor = {}, ver = {}, u = {}, v = {}, w = {};
    float fov_y = {}, aspect = {}, lens_radius = {}, focus_dist = {};
public:
    camera(void) = default;
    camera(float fovy, float aspect_ratio, float aperture, float focus_dist);
    void set_pos(vec3 pos, vec3 eye, vec3 up);
    ray ray_uv(float u, float v, Math::rnd_generator_t *rnd) const;
};

class tracer {
public:
    NNGN_MOVE_ONLY(tracer)
    tracer(void) = default;
    ~tracer(void);
    void init(Math *m) { this->math = m; }
    bool done(void) const { return this->n_samples == this->max_samples; }
    Camera *camera(void) { return this->m_camera; }
    void set_enabled(bool b) { this->flags.set(Flag::ENABLED, b); }
    void set_n_threads(std::size_t n) { this->n_threads = n; }
    void set_size(std::size_t w, std::size_t h);
    void set_max_depth(std::size_t n) { this->max_depth = n; }
    void set_max_samples(std::size_t n) { this->max_samples = n; }
    void set_min_t(float t) { this->min_t = t; }
    void set_max_t(float t) { this->max_t = t; }
    void set_max_lambertians(std::size_t n);
    void set_max_metals(std::size_t n);
    void set_max_dielectrics(std::size_t n);
    void set_camera(Camera *c);
    // XXX
    void set_camera_aperture(float a) { this->aperture = a; }
    void set_objects(world w) { this->objects = std::move(w); }
    lambertian *add_lambertian(auto &&...args);
    metal *add_metal(auto &&...args);
    dielectric *add_dielectric(auto &&...args);
    sphere *add_sphere(auto &&...args);
    void reset(void);
    bool update(const Timing &t);
    void write_tex(std::span<std::byte> s, bool gamma_correction) const;
private:
    enum Flag : u8 {
        ENABLED = 1 << 0,
        CAMERA_UPDATED = 1 << 1,
    };
    struct ThreadData {
        ThreadData(std::ptrdiff_t n) : begin_barrier{n}, end_barrier{n} {}
        ThreadPool pool = {};
        std::barrier<> begin_barrier, end_barrier;
    };
    Flags<Flag> flags = {};
    Math *math = nullptr;
    Camera *m_camera = nullptr;
    std::unique_ptr<ThreadData> thread_data = {};
    zvec2 size = {};
    std::size_t max_depth = 1, n_samples = 0, max_samples = 1;
    std::size_t n_threads = 0;
    float min_t = 0, max_t = 0, aperture = 0;
    u32 tex = 0;
    std::vector<float> img = {};
    std::vector<lambertian> lambertians = {};
    std::vector<metal> metals = {};
    std::vector<dielectric> dielectrics = {};
    world objects = {};
    nngn::ray::camera c = {};
    vec3 color(ray r) const;
    vec3 read_pixel(std::size_t x, std::size_t y);
    void write_pixel(std::size_t x, std::size_t y, const vec4 &c);
};

inline void tracer::set_camera(Camera *c_) {
    this->flags.set(Flag::CAMERA_UPDATED);
    this->m_camera = c_;
}

sphere *world::add_sphere(auto &&...args) {
    return &this->spheres.emplace_back(FWD(args)...);
}

lambertian *tracer::add_lambertian(auto &&...a) {
    return &this->lambertians.emplace_back(FWD(a)...);
}

metal *tracer::add_metal(auto &&...a) {
    return &this->metals.emplace_back(FWD(a)...);
}

dielectric *tracer::add_dielectric(auto &&...a) {
    return &this->dielectrics.emplace_back(FWD(a)...);
}

sphere *tracer::add_sphere(auto &&...a) {
    return this->objects.add_sphere(FWD(a)...);
}

}

#endif
