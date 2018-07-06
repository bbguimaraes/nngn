#ifndef NNGN_UTILS_CONCEPTS_H
#define NNGN_UTILS_CONCEPTS_H

#include <concepts>
#include <type_traits>
#include <version>

namespace nngn {

template<typename D, typename B>
concept derived_from =
#ifdef _LIBCPP_VERSION
    std::is_base_of_v<B, D>
    && std::is_convertible_v<const volatile D*, const volatile B*>;
#else
    std::derived_from<D, B>;
#endif

}

#endif
