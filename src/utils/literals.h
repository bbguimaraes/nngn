#ifndef NNGN_UTILS_LITERALS_H
#define NNGN_UTILS_LITERALS_H

#include <cstddef>
#include <cstdint>

#include "utils/fixed_string.h"

namespace nngn::literals {

inline constexpr auto operator ""_uc(unsigned long long int i) {
    return static_cast<unsigned char>(i);
}

inline constexpr auto operator ""_u8(unsigned long long int i) {
    return static_cast<std::uint8_t>(i);
}

inline constexpr auto operator ""_u16(unsigned long long int i) {
    return static_cast<std::uint16_t>(i);
}

inline constexpr auto operator ""_u32(unsigned long long int i) {
    return static_cast<std::uint32_t>(i);
}

inline constexpr auto operator ""_z(unsigned long long int i) {
    return static_cast<std::size_t>(i);
}

inline constexpr auto operator ""_t(unsigned long long int i) {
    return static_cast<std::ptrdiff_t>(i);
}

template<nngn::fixed_string s>
inline constexpr auto operator ""_s(void) {
    return s;
}

}

#endif
