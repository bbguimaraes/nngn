#include <cstring>

#include "utils/vector.h"

#include "ray.h"
#include "tracer.h"

namespace {

float *pixel(float *p, size_t w, size_t h, size_t x, size_t y)
    { return p + 4 * (w * (h - y - 1) + x); }
size_t tex_bytes(size_t w, size_t h) { return 4 * sizeof(float) * w * h; }

}

namespace nngn::ray {

bool world::hit(
        const ray &r, float t_min, float t_max, nngn::ray::hit *h) const {
    float t = t_max;
    nngn::ray::hit closest = {};
    for(const auto &x : this->spheres)
        if(x.hit(r, t_min, t, &closest) && closest.t < t)
            t = closest.t;
    if(t == t_max)
        return false;
    *h = closest;
    return true;
}

void camera::set_pos(
        vec3 pos, const vec3 &eye, const vec3 &up) {
    const float hh = std::tan(this->fovy / 2);
    const float hw = this->asp * hh;
    this->w = Math::normalize(pos - eye);
    this->u = Math::normalize(Math::cross(up, this->w));
    this->v = Math::cross(this->w, this->u);
    this->o = pos;
    this->bl = pos
        - hw * focus_dist * this->u
        - hh * focus_dist * this->v
        - focus_dist * this->w;
    this->hor = 2 * hw * focus_dist * this->u;
    this->ver = 2 * hh * focus_dist * this->v;
}

ray camera::ray_uv(float p_u, float p_v, Math::rnd_generator_t *rnd) const {
    const auto rd = this->lens_radius * sphere(1).random_point(rnd);
    const auto off = this->u * rd.x + this->v * rd.y;
    return ray{
        this->o + off,
        this->bl + p_u * this->hor + p_v * this->ver - this->o - off};
}

constexpr vec3 tracer::interp(const vec3 &v, const vec3 &u, float t)
    { return (1 - t) * v + t * u; }

vec3 tracer::color(ray r) const {
    auto *rnd = this->math->rnd_generator();
    vec3 color{1};
    for(size_t d = 0;; ++d) {
        if(d == this->max_depth)
            return {};
        hit h = {};
        if(!this->objects.hit(r, this->min_t, this->max_t, &h))
            break;
        const auto [ret, scattered, att] = h.mat->scatter(r, h, rnd);
        if(!ret)
            return {};
        color *= att;
        r = scattered;
    }
    return color * tracer::interp(
        {1, 1, 1}, {0.5, 0.7f, 1},
        0.5f * (Math::normalize(r.d).y + 1.0f));
}

vec3 tracer::read_pixel(size_t x, size_t y) {
    auto *p = pixel(this->tex.data(), this->img_width, this->img_height, x, y);
    return {p[0], p[1], p[2]};
}

void tracer::write_pixel(size_t x, size_t y, const vec4 &color) {
    std::memcpy(
        pixel(this->tex.data(), this->img_width, this->img_height, x, y),
        &color[0], sizeof(color));
}

void tracer::set_max_lambertians(size_t n)
    { set_capacity(&this->lambertians, n); }
void tracer::set_max_metals(size_t n)
    { set_capacity(&this->metals, n); }
void tracer::set_max_dielectrics(size_t n)
    { set_capacity(&this->dielectrics, n); }

void tracer::init(Math *m, size_t w, size_t h) {
    this->math = m;
    this->img_width = w;
    this->img_height = h;
    this->c = {
        Math::radians(20.0f),
        static_cast<float>(w) / static_cast<float>(h),
        0.1f, 10.0f};
    this->c.set_pos({13, 2, 3}, {0, 0, 0}, {0, 1, 0});
    this->tex.resize(4 * w * h);
}

void tracer::reset() {
    this->n_samples = 0;
    memset(this->tex.data(), 0, tex_bytes(this->img_width, this->img_height));
}

bool tracer::trace(const Timing &t) {
    if(!this->flags.is_set(Flag::ENABLED))
        return false;
    if(this->m_camera.update(t))
        this->reset();
    if(this->n_samples == this->max_samples)
        return false;
    const auto eye = this->m_camera.p + (
        Math::rotate(mat4(1), -this->m_camera.r.x, {1, 0, 0})
            * Math::rotate(mat4(1), -this->m_camera.r.y, {0, 1, 0})
            * Math::rotate(mat4(1), -this->m_camera.r.z, {0, 0, 1})
            * vec4(0, 0, -1, 1)).xyz();
    this->c.set_pos(this->m_camera.p, eye, {0, 1, 0});
    auto gen = this->math->rnd_generator();
    auto rnd =
        [gen, d = std::uniform_real_distribution<float>()]() mutable
        { return d(*gen); };
    const auto fi = static_cast<float>(n_samples);
    for(size_t y = 0; y < this->img_height; ++y) {
        const auto v =
            (static_cast<float>(y) + rnd())
            / static_cast<float>(this->img_height);
        for(size_t x = 0; x < this->img_width; ++x) {
            const auto u =
                (static_cast<float>(x) + rnd())
                / static_cast<float>(this->img_width);
            const auto prev_pixel = this->read_pixel(x, y) * fi;
            const auto pixel_i = this->color(this->c.ray_uv(u, v, gen));
            this->write_pixel(x, y, {(pixel_i + prev_pixel) / (fi + 1), 1});
        }
    }
    ++this->n_samples;
    return true;
}

void tracer::write_tex(size_t w, std::byte *p) const {
    const auto pad = 4 * (w - this->img_width);
    auto b = this->tex.data();
    for(size_t y = 0; y < this->img_height; ++y) {
        for(size_t x = 0, xe = 4 * this->img_width; x < xe; ++x)
            *p++ = static_cast<std::byte>(
                static_cast<uint8_t>(std::sqrt(*b++) * 255.9f));
        p += pad;
    }
}

}
