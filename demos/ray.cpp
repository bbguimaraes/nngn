#include <fstream>
#include <iostream>
#include <vector>

#include "math/camera.h"
#include "math/math.h"
#include "ray/tracer.h"
#include "timing/timing.h"

namespace {

void write_img(
        std::ostream *s, size_t w, size_t h, const std::byte *v) {
    *s << "P3\n" << w << ' ' << h << "\n255\n";
    for(size_t y = 0; y < h; ++y)
        for(size_t x = 0; x < w; ++x) {
            const auto i = 4 * (w * y + x);
            *s << static_cast<unsigned>(v[i]) << ' '
                << static_cast<unsigned>(v[i + 1]) << ' '
                << static_cast<unsigned>(v[i + 2]) << '\n';
        }
}

void init(nngn::ray::tracer *tracer, nngn::Math *math, size_t spread) {
    using nngn::vec3;
    constexpr size_t n = 500;
    tracer->set_max_lambertians(n + 2);
    tracer->set_max_metals(n + 1);
    tracer->set_max_dielectrics(n + 1);
    auto rnd = [g = math->rnd_generator()]() mutable
        { return std::uniform_real_distribution<float>()(*g); };
    std::vector<nngn::ray::sphere> spheres = {};
    spheres.reserve(n + 4);
    const auto spread_i = static_cast<int>(spread);
    for(int a = -spread_i; a < spread_i; ++a)
        for(int b = -spread_i; b < spread_i; ++b) {
            const vec3 sc = {
                static_cast<float>(a) + .9f * rnd(), .2f,
                static_cast<float>(b) + .9f * rnd()};
            if(nngn::Math::length(sc - vec3(4, .2f, 0)) <= .9f)
                continue;
            if(const auto choice = rnd(); choice < .8f)
                spheres.emplace_back(
                    sc, .2f, tracer->add_lambertian(
                        vec3(rnd(), rnd(), rnd())
                        * vec3(rnd(), rnd(), rnd())));
            else if(choice < .95f)
                spheres.emplace_back(
                    sc, .2f, tracer->add_metal(
                        .5f * (vec3(1) + vec3(rnd(), rnd(), rnd())),
                        .5f * rnd()));
            else
                spheres.emplace_back(sc, .2, tracer->add_dielectric(1.5));
        }
    spheres.emplace_back(vec3(0, 1, 0), 1, tracer->add_dielectric(1.5f));
    spheres.emplace_back(
        vec3(-4, 1, 0), 1, tracer->add_lambertian(vec3(0.4f, 0.2f, 0.1f)));
    spheres.emplace_back(
        vec3(4, 1, 0), 1, tracer->add_metal(vec3(0.7f, 0.6f, 0.5f), 0));
    spheres.emplace_back(
        vec3(0, -1000, 0), 1000.0f, tracer->add_lambertian(vec3(.5)));
    tracer->set_objects(nngn::ray::world(std::move(spheres)));
}

}

int main(int argc, const char *const *argv) {
    if(argc < 4) {
        std::cerr << "Usage: " << argv[0] << " width height spread samples\n";
        return 1;
    }
    const size_t
        w = std::stoul(argv[1]),
        h = std::stoul(argv[2]),
        spread = std::stoul(argv[3]),
        samples = std::stoul(argv[4]);
    nngn::Math math;
    math.init();
    nngn::ray::tracer tracer;
    tracer.init(&math, w, h);
    tracer.set_min_t(0.001f);
    tracer.set_max_t(9999999);
    tracer.set_max_samples(samples);
    tracer.camera()->look_at({}, {13, 2, 3}, {0, 1, 0});
    init(&tracer, &math, spread);
    for(const nngn::Timing t = {}; tracer.trace(t););
    std::vector<std::byte> tex(4 * w * h);
    tracer.write_tex(w, tex.data());
    std::ofstream f("ray.ppm");
    write_img(&f, w, h, tex.data());
}
