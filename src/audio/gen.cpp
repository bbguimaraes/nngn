#include "audio.h"

#include <cfloat>
#include <cmath>
#include <numbers>

#include "utils/ranges.h"

namespace {

constexpr auto pi = std::numbers::pi_v<float>;
constexpr auto w = 2.0f * pi;

void fade(std::span<float> s, std::size_t n, float g0, float g1, auto &&f) {
    const auto sn = s.size();
    const auto nf = static_cast<float>(n);
    for(std::size_t i = 0; i != sn; ++i)
        s[i] *= std::lerp(g0, g1, f(static_cast<float>(i) / nf));
}

void sine(std::span<float> s, std::size_t rate, auto &&f) {
    const auto t = w / static_cast<float>(rate);
    for(std::size_t i = 0; i != s.size(); ++i)
        s[i] += std::sin(f(static_cast<float>(i) * t));
}

}

namespace nngn {

void Audio::gain(std::span<float> s, float g) {
    for(auto &x : s)
        x *= g;
}

void Audio::fade(std::span<float> s, float g0, float g1) {
    return ::fade(s, s.size(), g0, g1, std::identity{});
}

void Audio::exp_fade(
    std::span<float> s, std::size_t ep, float g0, float g1, float e
) {
    const auto f = [e](auto x) { return std::pow(x, e); };
    if(g0 <= g1)
        return ::fade(s, ep, g0, g1, f);
    return ::fade(s, ep, g1, g0, [f](auto x) { return f(1 - x); });
}

void Audio::env(
    std::span<float> s,
    std::size_t a, std::size_t d, float st, std::size_t r
) {
    const auto as = slice(s, 0, a);
    const auto ds = slice(s, as.size(), as.size() + d);
    const auto rs = slice(
        s, ssize(s) - static_cast<std::ptrdiff_t>(r), s.size());
    const auto ss = slice(s, as.size() + ds.size(), ssize(s) - ssize(rs));
    Audio::fade(as, 0, 1);
    Audio::fade(ds, 1, st);
    Audio::gain(ss, st);
    Audio::fade(rs, st, 0);
}

void Audio::mix(std::span<float> dst, std::span<const float> src) {
    const auto n = src.size();
    assert(n <= dst.size());
    for(std::size_t i = 0; i != n; ++i)
        dst[i] += src[i];
}

void Audio::normalize(std::span<i16> dst, std::span<const float> src) {
    const auto n = dst.size();
    assert(n == src.size());
    constexpr auto max = static_cast<float>(INT16_MAX);
    for(std::size_t i = 0; i != n; ++i)
        dst[i] = static_cast<i16>(max * src[i]);
}

void Audio::gen_sine(std::span<float> s, float freq) const {
    return ::sine(s, this->m_rate, [freq](auto t) { return freq * t; });
}

void Audio::gen_sine_fm(
    std::span<float> s, float freq, float lfo_a, float lfo_freq, float lfo_d
) const {
    return ::sine(s, this->m_rate, [freq, lfo_a, lfo_freq, lfo_d](auto t) {
        const auto fm = lfo_a * std::sin(lfo_freq * (t + lfo_d));
        return freq * (t + fm);
    });
}

void Audio::gen_square(std::span<float> s, float freq) const {
    const auto r = static_cast<float>(this->m_rate);
    for(std::size_t i = 0; i != s.size(); ++i)
        s[i] += std::fmod(static_cast<float>(i) * freq, r) < r / 2.0f
            ? 1.0f : -1.0f;
}

void Audio::gen_saw(std::span<float> s, float freq) const {
    const auto r = static_cast<float>(this->m_rate);
    for(std::size_t i = 0; i != s.size(); ++i)
        s[i] += 2.0f * std::fmod(static_cast<float>(i) * freq, r) / r - 1.0f;
}

void Audio::gen_noise(std::span<float> s) const {
    assert(this->math);
    auto *const rnd = this->math->rnd_generator();
    auto dist = std::uniform_real_distribution<float>{
        -1.0f, 1.0f + FLT_EPSILON};
    for(auto &x : s)
        x += dist(*rnd);
}

}
