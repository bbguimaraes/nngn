#ifndef NNGN_UTILS_RANGES_H
#define NNGN_UTILS_RANGES_H

#include <algorithm>
#include <ranges>

#include "utils.h"

namespace nngn {

template<typename T>
class owning_view {
public:
    constexpr owning_view(const T&) = delete;
    constexpr explicit owning_view(T &r);
    constexpr explicit owning_view(T &&r) : owning_view{r} {}
    constexpr auto begin(void) const { return this->b; }
    constexpr auto begin(void) { return this->b; }
    constexpr auto end(void) const { return this->e; }
    constexpr auto end(void) { return this->e; }
private:
    std::ranges::iterator_t<T> b, e;
};

template<typename T>
inline constexpr owning_view<T>::owning_view(T &r) :
    b{std::move(std::ranges::begin(r))},
    e{std::move(std::ranges::end(r))}
{}

/* TODO libc++
constexpr auto enumerate(std::ranges::range auto &&r, auto i = std::size_t{}) {
    return std::views::transform(
        FWD(r), [i](auto &&x) mutable { return std::pair{i++, FWD(x)}; });
} */

template<std::ranges::forward_range R, typename Proj = std::identity>
constexpr bool is_sequence(R &&r, Proj proj = {}) {
    return std::adjacent_find(
        std::ranges::begin(FWD(r)), std::ranges::end(FWD(r)),
        [proj]<typename T>(const T &lhs, const T &rhs) {
            return proj(lhs) + proj(T{1}) != proj(rhs);
        }
    ) == end(FWD(r));
}

}

namespace std::ranges {

#ifdef DOXYGEN
template<typename T> inline constexpr bool enable_view;
template<typename T> inline constexpr bool enable_borrowed_range;
#endif

/** Makes \ref nngn::owning_view implement `std::ranges::view`. */
template<typename T>
inline constexpr bool enable_view<nngn::owning_view<T>> = true;

/** Makes \ref nngn::owning_view implement `std::ranges::borrowed_range`. */
template<typename T>
inline constexpr bool enable_borrowed_range<nngn::owning_view<T>> = true;

}

#endif
