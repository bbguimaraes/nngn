#ifndef NNGN_UTILS_SPAN_H
#define NNGN_UTILS_SPAN_H

#include <algorithm>
#include <cassert>
#include <span>
#include <string_view>

namespace nngn {

template<typename T>
std::span<T> slice(std::span<T> s, auto b, auto e) {
    using I = std::ptrdiff_t;
    const auto n = static_cast<I>(s.size());
    auto bi = static_cast<I>(b);
    auto ei = static_cast<I>(e);
    bi = std::clamp(bi, I{}, n);
    ei = std::clamp(ei, bi, n);
    bi = std::min(bi, ei);
    return s.subspan(
        static_cast<std::size_t>(bi),
        static_cast<std::size_t>(ei - bi));
}

template<typename T>
auto split(std::span<T> s, std::size_t i) {
    assert(i <= size(s));
    return std::pair{s.subspan(0, i), s.subspan(i)};
}

inline auto split(std::string_view s, std::string_view::iterator i) {
    return std::pair{
        std::string_view{begin(s), i},
        std::string_view{i, end(s)},
    };
}

}

#endif
