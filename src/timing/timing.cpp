#include "timing.h"

using ns = std::chrono::nanoseconds;
using us = std::chrono::microseconds;
using ms = std::chrono::milliseconds;
using s = std::chrono::seconds;
template<typename T>
using fd = std::chrono::duration<float, typename T::period>;
using rep = nngn::Timing::duration::rep;

namespace nngn {

template<typename D>
static auto now(const Timing &t) {
    return std::chrono::duration_cast<D>(t.now.time_since_epoch()).count();
}

rep Timing::now_ns(void) const { return nngn::now<ns>(*this); }
rep Timing::now_us(void) const { return nngn::now<us>(*this); }
rep Timing::now_ms(void) const { return nngn::now<ms>(*this); }
rep Timing::now_s(void) const { return nngn::now<s>(*this); }

template<typename D>
static auto fnow(const Timing &t) {
    return now<fd<D>>(t);
}
float Timing::fnow_ns(void) const { return fnow<ns>(*this); }
float Timing::fnow_us(void) const { return fnow<us>(*this); }
float Timing::fnow_ms(void) const { return fnow<ms>(*this); }
float Timing::fnow_s(void) const { return fnow<s>(*this); }

template<typename D>
static auto dt(const Timing &t) {
    return std::chrono::duration_cast<D>(t.dt).count();
}

rep Timing::dt_ns(void) const { return nngn::dt<ns>(*this); }
rep Timing::dt_us(void) const { return nngn::dt<us>(*this); }
rep Timing::dt_ms(void) const { return nngn::dt<ms>(*this); }
rep Timing::dt_s(void) const { return nngn::dt<s>(*this); }

template<typename D>
static auto fdt(const Timing &t) {
    return dt<fd<D>>(t);
}

float Timing::fdt_ns(void) const { return fdt<ns>(*this); }
float Timing::fdt_us(void) const { return fdt<us>(*this); }
float Timing::fdt_ms(void) const { return fdt<ms>(*this); }
float Timing::fdt_s(void) const { return fdt<s>(*this); }

void Timing::update(void) {
    ++this->frame;
    const auto old = this->now;
    this->now = Timing::clock::now();
    this->dt = this->now - old;
}

}
