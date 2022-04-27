#ifndef NNGN_UTILS_RANGES_H
#define NNGN_UTILS_RANGES_H

#include <algorithm>
#ifndef __clang__
#include <ranges>
#endif

#include "utils.h"

namespace nngn {

template<std::ranges::forward_range R, typename Proj = std::identity>
constexpr bool is_adjacent(R &&r, Proj proj = {}) {
    constexpr auto not_adj = []<typename T>(const T &lhs, const T &rhs) {
        return lhs + T{1} != rhs;
    };
    using std::end;
    return std::ranges::adjacent_find(FWD(r), not_adj, proj) == end(FWD(r));
}

}

#endif
