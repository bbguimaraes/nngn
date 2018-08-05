#ifndef NNGN_TIMING_PROFILE_H
#define NNGN_TIMING_PROFILE_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <tuple>

#include "utils/utils.h"

#include "stats.h"
#include "timing.h"

namespace nngn {

#define NNGN_STATS_CONTEXT(c, p) \
    const auto NNGN_STATS_CONTEXT_VAR(__LINE__) = nngn::Profile::context<c>(p);
#define NNGN_PROFILE_CONTEXT(p) \
    NNGN_STATS_CONTEXT(nngn::Profile, &nngn::Profile::stats.p)
#define NNGN_STATS_CONTEXT_VAR(l) NNGN_STATS_CONTEXT_JOIN(prof_, l)
#define NNGN_STATS_CONTEXT_JOIN(x, y) x##y

struct ProfileStats : StatsBase<ProfileStats, 2> {
    std::array<uint64_t, 2>
        schedule, socket, renderers, renderers_debug, render, vsync;
    static constexpr std::array names = {
        "schedule", "socket", "renderers", "renderers_debug", "render",
        "vsync",
    };
    const uint64_t *to_u64() const { return this->schedule.data(); }
    uint64_t *to_u64() { return this->schedule.data(); }
};

struct Profile {
    using Stats = ProfileStats;
    static constexpr size_t STATS_IDX = 0;
    template<typename T>
    struct context {
        static constexpr auto I = T::STATS_IDX;
        static constexpr auto N = T::Stats::N_EVENTS;
        std::array<uint64_t, N> *p;
        NNGN_MOVE_ONLY(context)
        explicit context(std::array<uint64_t, N> *p);
        ~context(void) { this->end(); }
        void end();
    };
    static ProfileStats prev, stats;
    static Timing::duration::rep sample();
    static void init();
    static void swap();
};

template<typename T>
inline Profile::context<T>::context(std::array<uint64_t, N> *p_) : p(p_) {
    if(nngn::Stats::active(I))
        this->p->front() = static_cast<uint64_t>(Profile::sample());
}

template<typename T>
inline void Profile::context<T>::end() {
    if(nngn::Stats::active(I))
        if(auto *const op = std::exchange(this->p, nullptr))
            op->back() = static_cast<uint64_t>(Profile::sample());
}

inline void Profile::init()
    { nngn::Stats::reserve(Profile::STATS_IDX, Profile::prev.to_u64_array()); }

inline void Profile::swap() { Profile::prev = Profile::stats; }

inline Timing::duration::rep Profile::sample()
    { return nngn::Timing::clock::now().time_since_epoch().count(); }

}

#endif
