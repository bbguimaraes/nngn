#ifndef NNGN_UTILS_FN_H
#define NNGN_UTILS_FN_H

#include <tuple>

#include "types.h"

namespace nngn {

namespace detail {

template<typename T> struct fn_args;

template<typename R, typename ...A>
struct fn_args<R(*)(A...)> { using type = types<A...>; };

}

template<auto f>
using fn_args = typename detail::fn_args<decltype(f)>::type;

/** Function object which converts its argument to T via `static_cast`. */
template<typename T>
struct to {
    constexpr T operator()(auto &&x) const { return static_cast<T>(FWD(x)); }
};

/** Function object which always produces the same value. */
template<auto x>
struct constant {
    constexpr auto operator()(auto&&...) const { return x; }
};

}

#endif
