#ifndef NNGN_UTILS_RANGES_H
#define NNGN_UTILS_RANGES_H

#include <algorithm>
#include <iterator>
#include <numeric>
#include <ranges>
#include <utility>

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

template<typename V>
void set_capacity(V *v, size_t n) {
    if(n < v->capacity())
        V{}.swap(*v);
    return v->reserve(n);
}

template<std::size_t N>
consteval auto to_array(const char *s) {
    return [&s]<auto ...I>(std::index_sequence<I...>) {
        return std::array{s[I]...};
    }(std::make_index_sequence<N>{});
}

template<std::size_t N>
consteval auto to_array(const char (&s)[N]) {
    return to_array<N - 1>(static_cast<const char*>(s));
}

template<std::size_t N>
consteval auto to_array(const std::ranges::contiguous_range auto &r) {
    return to_array<N>(std::data(r));
}

template<std::size_t N, std::ranges::view V>
consteval auto to_array(V &&v) {
    using T = std::ranges::range_value_t<V>;
    std::array<T, N> ret = {};
    std::copy_n(std::ranges::begin(v), N, begin(ret));
    return ret;
}

template<std::ranges::forward_range R, typename Proj = std::identity>
constexpr bool is_sequence(R &&r, Proj proj = {}) {
    return std::adjacent_find(
        std::ranges::begin(FWD(r)), std::ranges::end(FWD(r)),
        [proj]<typename T>(const T &lhs, const T &rhs) {
            return proj(lhs) + proj(T{1}) != proj(rhs);
        }
    ) == end(FWD(r));
}

template<std::ranges::range R>
constexpr void iota(R &&r, std::ranges::range_value_t<R> &&x = {}) {
    std::iota(std::ranges::begin(r), std::ranges::end(r), FWD(x));
}

constexpr auto reduce(std::ranges::range auto &&r) {
    return std::reduce(std::ranges::begin(r), std::ranges::end(r));
}

constexpr auto reduce(
    std::ranges::range auto &&r, auto &&init = {}, auto &&op = std::plus<>{})
{
    return std::reduce(
        std::ranges::begin(r), std::ranges::end(r),
        FWD(init), FWD(op));
}

template<
    std::input_iterator I,
    std::output_iterator<std::iter_value_t<I>> O>
constexpr void fill_with_pattern(I f, I l, O df, O dl) {
    std::generate(df, dl, [i = f, f, l](void) mutable {
        if(i == l)
            i = f;
        return *i++;
    });
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
