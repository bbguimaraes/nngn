#ifndef NNGN_MATH_HASH_H
#define NNGN_MATH_HASH_H

#include <span>
#include <string_view>

namespace nngn {

using Hash = std::size_t;

// TODO constexpr
inline Hash hash(std::span<const char> s)
    { return std::hash<std::string_view>{}({s.data(), s.size()}); }

}

#endif
