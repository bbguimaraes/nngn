#ifndef NNGN_UTILS_TUPLE_H
#define NNGN_UTILS_TUPLE_H

#include <tuple>

#include "utils/utils.h"

namespace nngn {

template<typename T>
auto tuple_tail(T &&t) {
    constexpr auto n = std::tuple_size_v<std::decay_t<T>>;
    return []<auto I, auto ...Is>(auto &&t_, std::index_sequence<I, Is...>) {
        return std::tuple{std::get<Is>(FWD(t_))...};
    }(FWD(t), std::make_index_sequence<n>());
}

}

#endif
