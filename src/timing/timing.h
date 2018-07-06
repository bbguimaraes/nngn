#ifndef NNGN_TIMING_TIMING_H
#define NNGN_TIMING_TIMING_H

#include <chrono>

#include "utils/def.h"

namespace nngn {

struct Timing {
    using clock = std::chrono::steady_clock;
    using time_point = clock::time_point;
    using duration = clock::duration;
    time_point now = clock::now();
    u64 frame = 0;
    duration dt = duration(0);
    template<typename F> static duration time(F &&f);
    duration::rep now_ns() const;
    duration::rep now_us() const;
    duration::rep now_ms() const;
    duration::rep now_s() const;
    float fnow_ns() const;
    float fnow_us() const;
    float fnow_ms() const;
    float fnow_s() const;
    duration::rep dt_ns() const;
    duration::rep dt_us() const;
    duration::rep dt_ms() const;
    duration::rep dt_s() const;
    float fdt_ns() const;
    float fdt_us() const;
    float fdt_ms() const;
    float fdt_s() const;
    void update();
};

template<typename F> Timing::duration Timing::time(F &&f) {
    const auto t0 = Timing::clock::now();
    f();
    return Timing::clock::now() - t0;
}

}

#endif
