#include <algorithm>
#include <cstring>

#include "graphics/texture.h"
#include "utils/vector.h"

#include "ray.h"
#include "tracer.h"

#include "utils/log.h"

namespace {

template<typename T>
constexpr decltype(auto) interp(const T &v, const T &u, float t) {
    return (1 - t) * v + t * u;
}

float *pixel(float *p, nngn::zvec2 size, nngn::zvec2 pos) {
    return p + 4 * (size.x * (size.y - pos.y - 1) + pos.x);
}

}

namespace nngn::ray {

bool world::hit(ray r, float t_min, float t_max, nngn::ray::hit *h) const {
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

camera::camera(float fov_y_, float aspect_, float aperture, float focus_dist_) :
    fov_y{fov_y_},
    aspect{aspect_},
    lens_radius{aperture / 2},
    focus_dist{focus_dist_} {}

void camera::set_pos(vec3 pos, vec3 eye, vec3 up) {
    const float half_h = std::tan(this->fov_y / 2);
    const float half_w = this->aspect * half_h;
    const auto d = eye - pos;
    const auto focal_len = Math::length(d);
    this->w = d / focal_len;
    this->u = Math::normalize(Math::cross(up, this->w));
    this->v = Math::cross(this->w, this->u);
    this->o = eye;
    this->hor = 2 * half_w * focus_dist * this->u;
    this->ver = 2 * half_h * focus_dist * this->v;
    this->bl = eye
        - this->hor / 2.0f
        - this->ver / 2.0f
        - this->w * focus_dist;
}

ray camera::ray_uv(float p_u, float p_v, Math::rnd_generator_t *rnd) const {
    const auto rd = this->lens_radius * sphere(1).random_point(rnd);
    const auto off = this->u * rd.x + this->v * rd.y;
    return ray{
        this->o + off,
        this->bl + p_u * this->hor + p_v * this->ver - this->o - off};
}

tracer::~tracer(void) {
    if(!this->thread_data)
        return;
    this->thread_data->pool.stop();
    this->thread_data->begin_barrier.arrive_and_wait();
}

vec3 tracer::color(ray r) const {
    auto *rnd = this->math->rnd_generator();
    auto color = vec3{1};
    for(std::size_t d = 0;; ++d) {
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
    return color * interp(
        vec3{1, 1, 1}, vec3{0.5, 0.7f, 1},
        0.5f * (Math::normalize(r.d).y + 1.0f));
}

vec3 tracer::read_pixel(std::size_t x, std::size_t y) {
    auto *p = pixel(this->img.data(), this->size, {x, y});
    return {p[0], p[1], p[2]};
}

void tracer::write_pixel(std::size_t x, std::size_t y, const vec4 &color) {
    std::memcpy(
        pixel(this->img.data(), this->size, {x, y}),
        &color[0], sizeof(color));
}

void tracer::set_size(std::size_t w, std::size_t h) {
    this->size = {w, h};
    this->img.resize(4 * w * h);
}

void tracer::set_max_lambertians(std::size_t n) {
    set_capacity(&this->lambertians, n);
}

void tracer::set_max_metals(std::size_t n) {
    set_capacity(&this->metals, n);
}

void tracer::set_max_dielectrics(std::size_t n) {
    set_capacity(&this->dielectrics, n);
}

void tracer::reset(void) {
    this->n_samples = 0;
    std::ranges::fill(this->img, 0);
}

bool tracer::update(const Timing &t) {
    if(!this->flags.is_set(Flag::ENABLED))
        return false;
    if(this->m_camera && this->m_camera->update(t)) {
        this->reset();
        this->flags.set(Flag::CAMERA_UPDATED);
    }
    if(this->n_samples == this->max_samples)
        return false;
    if(auto *const camera = this->m_camera) {
        const auto [sx, sy] = static_cast<vec2>(camera->screen);
        if(this->flags.check_and_clear(Flag::CAMERA_UPDATED)) {
            const auto aspect = sx / sy;
            this->c = {camera->fov_y, aspect, this->aperture, camera->zoom};
            this->n_samples = 0;
        }
        this->c.set_pos(camera->pos(), camera->eye(), camera->up());
    }
    const auto line = [this](auto *gen, std::size_t y) {
        auto rnd = std::uniform_real_distribution<float>{};
        const auto fi = static_cast<float>(this->n_samples);
        const auto size_f = static_cast<vec2>(this->size);
        for(size_t x = 0; x < this->size.x; ++x) {
            const auto v = (static_cast<float>(y) + rnd(*gen)) / size_f.y;
            const auto u = (static_cast<float>(x) + rnd(*gen)) / size_f.x;
            const auto prev_pixel = this->read_pixel(x, y) * fi;
            const auto pixel_i = this->color(this->c.ray_uv(u, v, gen));
            this->write_pixel(x, y, {(pixel_i + prev_pixel) / (fi + 1), 1});
        }
    };
    auto &rnd = *this->math->rnd_generator();
    if(this->n_threads && !this->thread_data) {
        this->thread_data = std::make_unique<ThreadData>(
            static_cast<std::ptrdiff_t>(this->n_threads) + 1);
        for(std::size_t i = 0; i < this->n_threads; ++i)
            this->thread_data->pool.start([
                this, line, i,
                data = this->thread_data.get(),
                rnd_i = Math::rnd_generator_t(rnd())
            ](std::stop_token stop) mutable {
                const auto n = this->size.y / n_threads;
                const auto b = n * i;
                const auto e = std::min(this->size.y, b + n);
                for(;;) {
                    data->begin_barrier.arrive_and_wait();
                    if(stop.stop_requested())
                        return;
                    for(auto y = b; y < e; ++y)
                        line(&rnd_i, y);
                    data->end_barrier.arrive_and_wait();
                }
            });
    }
    if(this->thread_data) {
        this->thread_data->begin_barrier.arrive_and_wait();
        this->thread_data->end_barrier.arrive_and_wait();
    } else for(std::size_t y = 0; y < this->size.y; ++y)
        line(&rnd, y);
    ++this->n_samples;
    return true;
}

void tracer::write_tex(std::span<std::byte> s, bool gamma_correction) const {
    assert(this->img.size() <= s.size());
    constexpr auto m = 255.99f;
    if(gamma_correction)
        std::ranges::transform(this->img, begin(s), [](auto x) {
            return static_cast<std::byte>(static_cast<u8>(std::sqrt(x) * m));
        });
    else
        std::ranges::transform(this->img, begin(s), [](auto x) {
            return static_cast<std::byte>(static_cast<u8>(x * m));
        });
}

}
