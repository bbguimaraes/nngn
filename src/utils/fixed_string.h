#ifndef NNGN_UTILS_FIXED_STRING_H
#define NNGN_UTILS_FIXED_STRING_H

#include <algorithm>
#include <cstddef>
#include <string_view>

namespace nngn {

template<std::size_t N>
struct fixed_string {
    constexpr fixed_string(void) = default;
    constexpr fixed_string(const char (&s_)[N]) {
        std::ranges::copy(s_, this->s);
    }
    constexpr operator std::string_view(void) const { return {this->s, N}; }
    constexpr std::size_t size(void) const { return N; }
    constexpr const char *data(void) const { return this->s; }
    char s[N] = {};
};

template<std::size_t N>
fixed_string(const char (&)[N]) -> fixed_string<N>;

}

#endif
