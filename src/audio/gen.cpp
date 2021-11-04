#include "audio.h"

#include <cfloat>
#include <cmath>
#include <numbers>

#include "utils/span.h"

namespace {

constexpr auto pi = std::numbers::pi_v<float>;
constexpr auto w = 2.0f * pi;

void fade(std::span<float> s, std::size_t ep, float g0, float g1, auto &&f) {
    for(std::size_t i = 0, n = s.size(); i != n; ++i) {
        const auto t = static_cast<float>(i) / static_cast<float>(ep);
        s[i] *= std::lerp(g0, g1, f(t));
    }
}

void sine(std::span<float> s, std::size_t rate, auto &&f) {
    const auto t = w / static_cast<float>(rate);
    for(std::size_t i = 0, n = s.size(); i != n; ++i)
        s[i] += std::sin(f(static_cast<float>(i) * t));
}

}

namespace nngn {

void Audio::gain(std::span<float> s, float g) {
    for(auto &x : s)
        x *= g;
}

void Audio::over(std::span<float> s, float m, float mix) {
    for(auto &x : s) {
        const auto o = -std::copysign(1.0f, x)
            * (1.0f - std::exp(-std::abs(m * x)));
        x = std::lerp(x, o, mix);
    }
}

void Audio::fade(std::span<float> s, float g0, float g1) {
    ::fade(s, s.size(), g0, g1, std::identity{});
}

void Audio::exp_fade(
    std::span<float> s, std::size_t ep, float g0, float g1, float e
) {
    if(g0 <= g1)
        ::fade(s, ep, g0, g1, [e](auto x) { return std::pow(x, e); });
    else
        ::fade(s, ep, g1, g0, [e](auto x) { return std::pow(1 - x, e); });
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
    assert(src.size() <= dst.size());
    for(std::size_t i = 0, n = src.size(); i != n; ++i)
        dst[i] += src[i];
}

void Audio::normalize(std::span<i16> dst, std::span<const float> src) {
    assert(src.size() == dst.size());
    constexpr auto max = static_cast<float>(INT16_MAX);
    for(std::size_t i = 0, n = src.size(); i != n; ++i)
        dst[i] = static_cast<i16>(max * src[i]);
}

void Audio::trem(std::span<float> s, float a, float freq, float mix) const {
    const auto t = freq * w / static_cast<float>(this->m_rate);
    for(std::size_t i = 0, n = s.size(); i != n; ++i) {
        const auto tr = s[i] * a * std::sin(static_cast<float>(i) * t);
        s[i] = std::lerp(s[i], tr, mix);
    }
}

void Audio::gen_sine(std::span<float> s, float freq) const {
    ::sine(s, this->m_rate, [freq](auto t) { return freq * t; });
}

void Audio::gen_sine_fm(
    std::span<float> s, float freq, float lfo_a, float lfo_freq, float lfo_d
) const {
    ::sine(s, this->m_rate, [freq, lfo_a, lfo_freq, lfo_d](auto t) {
        const auto fm = lfo_a * std::sin(lfo_freq * (t + lfo_d));
        return freq * (t + fm);
    });
}

void Audio::gen_square(std::span<float> s, float freq) const {
    const auto r = static_cast<float>(this->m_rate);
    const auto m = r / 2.0f;
    for(std::size_t i = 0; i != s.size(); ++i)
        s[i] += std::copysign(
            1.0f, m - std::fmod(static_cast<float>(i) * freq, r));
}

void Audio::gen_saw(std::span<float> s, float freq) const {
    const auto r = static_cast<float>(this->m_rate);
    for(std::size_t i = 0; i != s.size(); ++i)
        s[i] += 2.0f * std::fmod(static_cast<float>(i) * freq, r) / r - 1.0f;
}

void Audio::gen_noise(std::span<float> s) const {
    assert(this->math);
    constexpr auto min = -1.0f;
    constexpr auto max = std::bit_cast<float>(std::bit_cast<u32>(1.0f) + 1);
    auto *const rnd = this->math->rnd_generator();
    auto dist = std::uniform_real_distribution{min, max};
    for(auto &x : s)
        x += dist(*rnd);
}

}
