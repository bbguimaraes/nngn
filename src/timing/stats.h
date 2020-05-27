#ifndef NNGN_TIMING_STATS_H
#define NNGN_TIMING_STATS_H

#include <array>
#include <bitset>
#include <cassert>
#include <type_traits>

#include "utils/pointer_flag.h"

namespace nngn {

template<typename CRTP, size_t N> struct StatsBase {
    static constexpr auto N_EVENTS = N;
    static constexpr size_t size() { return CRTP::names.size() * N_EVENTS; }
    auto *to_u64_array() const {
        static_assert(CRTP::size() == sizeof(CRTP) / sizeof(uint64_t));
        return reinterpret_cast
            <const std::array<uint64_t, CRTP::size()>*>
            (static_cast<const CRTP*>(this)->to_u64());
    }
    auto *to_u64_array() {
        return const_cast<std::array<uint64_t, CRTP::size()>*>(
            static_cast<const StatsBase*>(this)->to_u64_array());
    }
};

class Stats {
    static constexpr size_t MAX = 1;
    static std::array<pointer_flag<void>, MAX> v;
    static void check([[maybe_unused]] size_t i)
        { assert(i < MAX); assert(v[i].get()); }
public:
    static void reserve(size_t i, void *p)
        { assert(i < MAX); assert(!v[i].get()); v[i] = pointer_flag(p); }
    static void release(size_t i) { check(i); v[i] = {}; }
    static bool active(size_t i) { assert(i < MAX); return v[i].flag(); }
    static void set_active(size_t i, bool a) { check(i); v[i].set_flag(a); }
    static void *data(size_t i) { check(i); return v[i].get(); }
    template<typename T> static auto *u64_data();
};

template<typename T>
inline auto *Stats::u64_data() {
    using S = typename T::Stats;
    constexpr auto I = T::STATS_IDX;
    return static_cast<S*>(Stats::data(I))->to_u64_array();
}

}

#endif
