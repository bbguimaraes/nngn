#ifndef NNGN_FLAGS_H
#define NNGN_FLAGS_H

#include <type_traits>
#include <utility>

#include "concepts/fundamental.h"
#include "utils.h"

namespace nngn {

/**
 * Wrapper for a small unsigned integral representing flags.
 * Operations cast the result back to the underlying type to remove the need for
 * explicit casts when it is converted to `int`.
 */
template<typename T>
struct Flags {
    /** Value type stored in the object. */
    using UT = std::underlying_type_t<T>;
    /** Accepted type for integral arguments. */
    using AT = std::conditional_t<scoped_enum<T>, T, UT>;
    /** Promoted type after an operation involving <tt>T</tt>s. */
    using PT = std::common_type_t<UT, int>;
    /** Creates an object from the result of an expression. */
    static constexpr Flags cast(PT x) { return static_cast<UT>(x); }
    // Constructors
    constexpr Flags(void) = default;
    constexpr Flags(UT u) : v{u} {}
    constexpr Flags(AT a) requires(scoped_enum<T>) : Flags{to_underlying(a)} {}
    // Conversion
    constexpr operator T(void) const { return static_cast<T>(this->v); }
    // Access
    constexpr T operator*(void) const { return T{*this}; }
    // Operations
    constexpr Flags operator~(void) const { return Flags::cast(~this->v); }
    constexpr Flags operator-(void) const { return Flags::cast(-this->v); }
    constexpr Flags operator&(AT rhs) { return *this & Flags{rhs}; }
    constexpr Flags operator|(AT rhs) { return *this | Flags{rhs}; }
    constexpr Flags operator^(AT rhs) { return *this ^ Flags{rhs}; }
    constexpr Flags &operator&=(Flags rhs) { return *this = *this & rhs; }
    constexpr Flags &operator|=(Flags rhs) { return *this = *this | rhs; }
    constexpr Flags &operator^=(Flags rhs) { return *this = *this ^ rhs; }
    // Methods
    constexpr bool is_set(AT a) const { return (*this & Flags{a}).v; }
    constexpr Flags &comp(void) { return *this = ~*this; }
    constexpr Flags &set(AT a) { return *this |= a; }
    constexpr Flags &set(AT a, bool b);
    constexpr Flags &clear(AT a) { return *this &= ~Flags{a}; }
    constexpr Flags check_and_clear(AT a);
    // Data
    UT v = {};
};

template<typename T>
constexpr Flags<T> operator&(Flags<T> lhs, Flags<T> rhs) {
    return lhs.v & rhs.v;
}

template<typename T>
constexpr Flags<T> operator|(Flags<T> lhs, Flags<T> rhs) {
    return lhs.v | rhs.v;
}

template<typename T>
constexpr Flags<T> operator^(Flags<T> lhs, Flags<T> rhs) {
    return lhs.v ^ rhs.v;
}

template<typename T>
constexpr auto Flags<T>::set(AT a, bool b) -> Flags& {
    return *this = Flags::cast(set_bit(this->v, a, b));
}

template<typename T>
constexpr auto Flags<T>::check_and_clear(AT a) -> Flags {
    return std::exchange(*this, *this & ~Flags{a}) & a;
}

}

#endif
