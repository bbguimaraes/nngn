#include "math.h"

#include <algorithm>
#include <cfloat>

#include "utils/log.h"

namespace nngn {

void Math::gaussian_filter(
    std::size_t size, float std_dev, std::span<float> s)
{
    assert(size <= s.size());
    const auto sd2 = std_dev * std_dev;
    const auto mul = 1.0f / std::sqrt(2.0f * Math::pi<float>() * sd2);
    const auto exp_mul = -1.0f / (2.0f * sd2);
    const auto hs = static_cast<int>(size / 2);
    auto *p = s.data();
    for(int i = -hs; i <= hs; ++i)
        *p++ = mul * std::pow(
            Math::e<float>(),
            exp_mul * static_cast<float>(i * i));
}

void Math::gaussian_filter(
    std::size_t xsize, std::size_t ysize, float std_dev, std::span<float> s)
{
    assert(xsize * ysize <= s.size());
    const auto sd2 = std_dev * std_dev;
    const auto mul = 1.0f / (2.0f * Math::pi<float>() * sd2);
    const auto exp_mul = -1.0f / (2.0f * sd2);
    const auto hsx = static_cast<int>(xsize / 2);
    const auto hsy = static_cast<int>(ysize / 2);
    auto *p = s.data();
    for(int y = -hsy; y <= hsy; ++y) {
        const auto y2 = static_cast<float>(y * y);
        for(int x = -hsx; x <= hsx; ++x) {
            const auto x2 = static_cast<float>(x * x);
            *p++ = mul * std::pow(Math::e<float>(), exp_mul * (x2 + y2));
        }
    }
}

void Math::mat_mul(
    std::span<float> dst, const float *src0, const float *src1, std::size_t n)
{
    assert(n * n <= dst.size());
    for(std::size_t i = 0; i != n; ++i)
        for(std::size_t j = 0; j != n; ++j) {
            dst[i * n + j] = 0;
            for(std::size_t k = 0; k != n; k++)
                dst[i * n + j] += src0[i * n + k] * src1[k * n + j];
        }
}

void Math::init() { this->m_rnd_generator.emplace(std::random_device{}()); }

void Math::seed_rand(rand_seed_t s) {
    NNGN_LOG_CONTEXT_CF(Math);
    if(!this->m_rnd_generator) {
        Log::l() << "RNG uninitialized\n";
        return;
    }
    this->m_rnd_generator->seed(s);
}

void Math::rand_mat(std::span<float> m) {
    // `double` since uniform_real_distribution requires that `b - a` be
    // representable.
    auto dist = std::uniform_real_distribution<double>{
        static_cast<double>(-FLT_MAX), std::nextafter(FLT_MAX, HUGE_VAL),
    };
    auto &gen = *this->m_rnd_generator;
    std::generate(
        begin(m), end(m),
        [dist, &gen](void) mutable { return static_cast<float>(dist(gen)); });
}

}
