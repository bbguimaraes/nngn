#ifndef NNGN_TIMING_STATS_H
#define NNGN_TIMING_STATS_H

#include <array>
#include <bitset>
#include <cassert>
#include <type_traits>

#include "utils/def.h"
#include "utils/pointer_flag.h"

namespace nngn {

template<typename CRTP, std::size_t N>
struct StatsBase {
    static constexpr auto N_EVENTS = N;
    static constexpr std::size_t size(void);
    auto *to_u64_array(void) const;
    auto *to_u64_array(void);
};

class Stats {
    static constexpr std::size_t MAX = 1;
    static std::array<pointer_flag<void>, MAX> v;
    static std::size_t check(std::size_t i);
public:
    static void reserve(std::size_t i, void *p);
    static void release(std::size_t i) { v[check(i)] = {}; }
    static bool active(std::size_t i) { assert(i < MAX); return v[i].flag(); }
    static void set_active(std::size_t i, bool a) { v[check(i)].set_flag(a); }
    static void *data(std::size_t i) { return v[check(i)].get(); }
    template<typename T> static auto *u64_data(void);
};

template<typename CRTP, std::size_t N>
constexpr std::size_t StatsBase<CRTP, N>::size(void) {
    return CRTP::names.size() * N_EVENTS;
}

template<typename CRTP, std::size_t N>
auto *StatsBase<CRTP, N>::to_u64_array(void) const {
    constexpr auto n = CRTP::size();
    static_assert(n == sizeof(CRTP) / sizeof(u64));
    using T = const std::array<u64, n>*;
    return reinterpret_cast<T>(static_cast<const CRTP&>(*this).to_u64());
}

template<typename CRTP, std::size_t N>
auto *StatsBase<CRTP, N>::to_u64_array(void) {
    auto &ret = *static_cast<const StatsBase*>(this)->to_u64_array();
    return &const_cast<std::decay_t<decltype(ret)>&>(ret);
}

inline std::size_t Stats::check(std::size_t i) {
    assert(i < MAX);
    assert(v[i].get());
    return i;
}

inline void Stats::reserve(std::size_t i, void *p) {
    assert(i < MAX);
    assert(!v[i].get());
    v[i] = pointer_flag(p);
}

template<typename T>
inline auto *Stats::u64_data() {
    using S = typename T::Stats;
    constexpr auto I = T::STATS_IDX;
    return static_cast<S*>(Stats::data(I))->to_u64_array();
}

}

#endif
