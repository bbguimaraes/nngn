/**
 * \dir src/timing
 * \brief Clocks, timers, profiling.
 *
 * This module centralizes time management.  Here the program's clock and
 * duration types are defined.  The main type, \ref nngn::Timing, also holds the
 * canonical frame counter, start-of-frame timestamp, and frame time step.
 * Where possible, all other components should update themselves with the values
 * contained in this object instead of using their own timers.
 */
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
    float scale = 1.0f;
    template<typename F> static duration time(F &&f);
    duration::rep now_ns(void) const;
    duration::rep now_us(void) const;
    duration::rep now_ms(void) const;
    duration::rep now_s(void) const;
    float fnow_ns(void) const;
    float fnow_us(void) const;
    float fnow_ms(void) const;
    float fnow_s(void) const;
    duration::rep dt_ns(void) const;
    duration::rep dt_us(void) const;
    duration::rep dt_ms(void) const;
    duration::rep dt_s(void) const;
    float fdt_ns(void) const;
    float fdt_us(void) const;
    float fdt_ms(void) const;
    float fdt_s(void) const;
    void update(void);
};

template<typename F> Timing::duration Timing::time(F &&f) {
    const auto t0 = Timing::clock::now();
    f();
    return Timing::clock::now() - t0;
}

}

#endif
