#ifndef NNGN_UTILS_FN_H
#define NNGN_UTILS_FN_H

#include "utils.h"

namespace nngn {

/** Function object which converts its argument to T via `static_cast`. */
template<typename T>
struct to {
    constexpr T operator()(auto &&x) const { return static_cast<T>(FWD(x)); }
};

}

#endif
