#include "math.h"

#include "utils/log.h"

namespace nngn {

void Math::gaussian_filter(std::size_t size, float std_dev, float *p) {
    const auto sd2 = std_dev * std_dev;
    const auto mul = 1.0f / std::sqrt(2.0f * Math::pi<float>() * sd2);
    const auto exp_mul = -1.0f / (2.0f * sd2);
    const auto hs = static_cast<int>(size / 2);
    for(int i = -hs; i <= hs; ++i)
        *p++ = mul * std::pow(
            Math::e<float>(),
            exp_mul * static_cast<float>(i * i));
}

void Math::gaussian_filter(
    std::size_t xsize, std::size_t ysize, float std_dev, float *p
) {
    const auto sd2 = std_dev * std_dev;
    const auto mul = 1.0f / (2.0f * Math::pi<float>() * sd2);
    const auto exp_mul = -1.0f / (2.0f * sd2);
    const auto hsx = static_cast<int>(xsize / 2);
    const auto hsy = static_cast<int>(ysize / 2);
    for(int y = -hsy; y <= hsy; ++y) {
        const auto y2 = static_cast<float>(y * y);
        for(int x = -hsx; x <= hsx; ++x) {
            const auto x2 = static_cast<float>(x * x);
            *p++ = mul * std::pow(Math::e<float>(), exp_mul * (x2 + y2));
        }
    }
}

void Math::mat_mul(size_t n, const float *m0, const float *m1, float *m2) {
    for(size_t i = 0; i < n; ++i)
        for(size_t j = 0; j < n; ++j) {
            m2[i * n + j] = 0;
            for(size_t k = 0; k < n; k++)
                m2[i * n + j] += m0[i * n + k] * m1[k * n + j];
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

void Math::rand_mat(size_t n, float *m) {
    auto &gen = *this->m_rnd_generator;
    auto dist = std::uniform_real_distribution<float>(
        std::numeric_limits<float>::max());
    for(size_t i = 0, n2 = n * n; i < n2; ++i)
        m[i] = dist(gen);
}

}
