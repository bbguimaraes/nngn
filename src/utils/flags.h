#ifndef NNGN_FLAGS_H
#define NNGN_FLAGS_H

#include <type_traits>
#include <utility>

#include "utils/utils.h"

namespace nngn {

/**
 * Wrapper for an unsigned integral representing flags.
 * Operations cast the result back to the underlying type to remove the need for
 * explicit casts when it is converted to `int`.
 */
template<typename T>
struct Flags {
    using UT = std::underlying_type_t<T>;
    UT t;
    constexpr bool is_set(UT u) const { return static_cast<bool>(*this & u); }
    template<typename U> static constexpr Flags cast(U u);
    constexpr Flags &flip() { return *this = ~*this; }
    constexpr Flags &set(UT u) { return *this = *this | u; }
    constexpr Flags &set(UT u, bool b);
    constexpr Flags &clear(UT u) { return *this = *this & ~Flags{u}; }
    constexpr Flags check_and_clear(UT u);
    constexpr explicit operator bool() const { return t; }
    constexpr Flags operator~() const { return Flags::cast(~this->t); }
    constexpr Flags operator-() const { return Flags::cast(-this->t); }
    constexpr Flags operator&(UT u) const { return Flags::cast(this->t & u); }
    constexpr Flags operator|(UT u) const { return Flags::cast(this->t | u); }
    constexpr Flags operator^(UT u) const { return Flags::cast(this->t ^ u); }
    constexpr Flags &operator&=(UT u) { return *this = *this & Flags{u}; }
    constexpr Flags &operator|=(UT u) { return *this = *this | Flags{u}; }
    constexpr Flags &operator^=(UT u) { return *this = *this ^ Flags{u}; }
    constexpr Flags operator&(const Flags &f) const { return *this & f.t; }
    constexpr Flags operator|(const Flags &f) const { return *this | f.t; }
    constexpr Flags operator^(const Flags &f) const { return *this ^ f.t; }
    constexpr Flags &operator&=(const Flags &f) { return *this = *this & f.t; }
    constexpr Flags &operator|=(const Flags &f) { return *this = *this | f.t; }
    constexpr Flags &operator^=(const Flags &f) { return *this = *this ^ f.t; }
};

template<typename T>
template<typename U>
constexpr auto Flags<T>::cast(U u) -> Flags
    { return {static_cast<UT>(u)}; }

template<typename T>
constexpr auto Flags<T>::set(UT u, bool b) -> Flags&
    { return *this = Flags::cast(set_bit(this->t, u, b)); }

template<typename T>
constexpr auto Flags<T>::check_and_clear(UT u) -> Flags
    { return std::exchange(*this, *this & ~u) & u; }
}

#endif
