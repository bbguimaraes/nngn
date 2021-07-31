#ifndef NNGN_UTILS_RANGES_H
#define NNGN_UTILS_RANGES_H

#include <algorithm>
#include <cassert>
#include <cstring>
#include <iterator>
#include <numeric>
#ifndef __clang__
#include <ranges>
#endif
#include <span>
#include <utility>

#include "utils.h"

namespace nngn {

template<typename T>
class owning_view {
public:
    constexpr owning_view(const T&) = delete;
    constexpr explicit owning_view(T &r) :
        b{std::ranges::begin(r)}, e{std::ranges::end(r)} {}
    constexpr explicit owning_view(T &&r) : owning_view{r} {}
    constexpr auto begin(void) const { return this->b; }
    constexpr auto begin(void) { return this->b; }
    constexpr auto end(void) const { return this->e; }
    constexpr auto end(void) { return this->e; }
private:
    std::ranges::iterator_t<T> b, e;
};

template<typename T>
struct range_to {
    template<std::ranges::range R>
    friend auto operator|(R &&r, range_to) {
        T ret = {};
        if constexpr(std::ranges::sized_range<R>)
            ret.reserve(std::ranges::size(FWD(r)));
        for(auto &&x : FWD(r))
            ret.push_back(x);
        return ret;
    }
};

template<typename T>
std::span<T> slice(std::span<T> s, auto b, auto e) {
    using I = std::ptrdiff_t;
    const auto n = static_cast<I>(s.size());
    auto b_zd = static_cast<I>(b);
    auto e_zd = static_cast<I>(e);
    assert(b_zd <= e_zd);
    b_zd = std::clamp(b_zd, I{}, n);
    e_zd = std::clamp(e_zd, b_zd, n);
    return s.subspan(
        static_cast<std::size_t>(b_zd),
        static_cast<std::size_t>(e_zd - b_zd));
}

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
consteval auto to_array(const char (&s)[N])
    { return to_array<N - 1>(static_cast<const char*>(s)); }

template<std::size_t N>
consteval auto to_array(const std::ranges::contiguous_range auto &r)
    { return to_array<N>(std::data(r)); }

constexpr bool contains(
    const std::ranges::contiguous_range auto &r, const auto &x
) {
    const void *const b = &*std::ranges::cbegin(r);
    const void *const e = &*std::ranges::cend(r);
    const void *const p = &x;
    return b <= p && p < e;
}

template<std::ranges::forward_range R, typename Proj = std::identity>
constexpr bool is_adjacent(R &&r, Proj proj = {}) {
    constexpr auto not_adj = []<typename T>(const T &lhs, const T &rhs) {
        return lhs + T{1} != rhs;
    };
    using std::end;
    return std::ranges::adjacent_find(FWD(r), not_adj, proj) == end(FWD(r));
}

template<std::ranges::range R>
constexpr void iota(R &&r, std::ranges::range_value_t<R> &&x = {}) {
    std::iota(std::ranges::begin(r), std::ranges::end(r), FWD(x));
}

constexpr auto reduce(std::ranges::range auto &&r) {
    using std::begin, std::end;
    return std::reduce(begin(r), end(r));
}

void const_time_erase(auto *v, auto *p) {
    assert(contains(*v, *p));
    auto last = v->end() - 1;
    *p = std::move(*last);
    v->erase(last);
}

template<
    std::input_iterator I,
    std::output_iterator<std::iter_value_t<I>> O>
constexpr void fill_with_pattern(I f, I l, O df, O dl) {
    std::generate(df, dl, [i = f, f, l]() mutable {
        if(i == l)
            i = f;
        return *i++;
    });
}

constexpr void memcpy(void *dst, std::ranges::contiguous_range auto &&r) {
    // TODO https://bugs.llvm.org/show_bug.cgi?id=39663
    //std::memcpy(dst, data(r), std::span{r}.size_bytes());
    auto s = std::span{r};
    std::memcpy(dst, data(r), s.size_bytes());
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
