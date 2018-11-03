#ifndef NNGN_UTILS_TYPES_H
#define NNGN_UTILS_TYPES_H

#include "utils/utils.h"

namespace nngn {

namespace detail { struct types_tag; }

template<typename T>
concept type_list = std::derived_from<T, detail::types_tag>;

namespace detail {

struct types_tag {};
template<typename ...Ts> struct types_impl;
template<type_list T> struct types_first;
template<type_list T> struct types_last;
template<type_list T> using types_first_t = typename types_first<T>::type;
template<type_list T> using types_last_t = typename types_last<T>::type;
template<type_list T> using type = types_first<T>;
template<type_list T> using type_t = types_first_t<T>;

template<typename T, typename ...Ts>
struct types_first<types_impl<T, Ts...>> : std::type_identity<T> {};

template<typename ...Ts>
struct types_last<types_impl<Ts...>>
    : std::type_identity<types_first_t<decltype((..., types_impl<Ts>{}))>> {};

template<typename ...Ts>
struct types_impl : types_tag {
    constexpr void map(auto &&f) const { (..., FWD(f)(types_impl<Ts>{})); }
};

}

using detail::types_first, detail::types_first_t;
using detail::types_last, detail::types_last_t;
using detail::type, detail::type_t;

template<typename ...Ts> using types = detail::types_impl<Ts...>;

}

#endif
