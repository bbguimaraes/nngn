#ifndef NNGN_UTILS_FIXED_STRING_H
#define NNGN_UTILS_FIXED_STRING_H

#include <algorithm>
#include <cstddef>
#include <ranges>
#include <string_view>

#include "utils.h"

namespace nngn {

template<std::size_t N>
struct fixed_string {
    NNGN_DEFAULT_CONSTRUCT(fixed_string)
    constexpr fixed_string(const char (&s_)[N + 1]);
    constexpr ~fixed_string(void) = default;
    constexpr operator std::string_view(void) const { return {this->s, N}; }
    constexpr std::size_t size(void) const { return N; }
    constexpr const char *data(void) const { return this->s; }
    constexpr const char *begin(void) const { return this->s; }
    constexpr const char *end(void) const { return this->s + N; }
    char s[N] = {};
};
static_assert(std::ranges::contiguous_range<fixed_string<1>>);

template<std::size_t N>
fixed_string(const char (&)[N]) -> fixed_string<N - 1>;

template<std::size_t N>
constexpr fixed_string<N>::fixed_string(const char (&s_)[N + 1]) {
    std::copy(s_, s_ + N, this->s);
}

/** Concatenates multiple \ref fixed_string objects into a character array. */
template<std::ranges::sized_range ...Rs>
constexpr auto cat(const Rs &...rs) {
    using T = std::common_type_t<std::ranges::range_value_t<Rs>...>;
    constexpr auto n = (... + std::tuple_size_v<Rs>);
    std::array<T, n> ret = {};
    std::size_t i = 0;
    (..., (std::copy(begin(rs), end(rs), &ret[i]), i += rs.size()));
    return ret;
}

}

namespace std {

template<size_t N>
struct tuple_size<nngn::fixed_string<N>> : integral_constant<size_t, N> {};

}

#endif
