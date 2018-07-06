#ifndef NNGN_UTILS_VECTOR_H
#define NNGN_UTILS_VECTOR_H

#include <algorithm>
#include <cstring>
#include <iterator>
#include <numeric>
#include <ranges>
#include <span>
#include <utility>

namespace nngn {

template<typename V>
void set_capacity(V *v, size_t n) {
    if(n < v->capacity())
        V{}.swap(*v);
    return v->reserve(n);
}

template<typename T, typename V>
auto vector_linear_erase(V *v, T *p) {
    const auto i = p - v->data();
    assert(0 <= i && static_cast<typename V::size_type>(i) < v->size());
    auto it = v->begin() + i;
    auto last = v->end() - 1;
    *it = std::move(*last);
    v->erase(last);
    return v->begin() + i;
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

template<std::ranges::range R>
constexpr bool is_adjacent(const R &r) {
    constexpr auto not_adj = [](const auto &lhs, const auto &rhs) {
        using T = std::ranges::range_value_t<R>;
        return lhs + T{1} != rhs;
    };
    using std::end;
    return std::ranges::adjacent_find(r, not_adj) == end(r);
}

auto reduce(std::ranges::range auto &&r) {
    using std::begin, std::end;
    return std::reduce(begin(r), end(r));
}

template<
    std::input_iterator I,
    std::output_iterator<std::iter_value_t<I>> O>
void fill_with_pattern(I f, I l, O df, O dl) {
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

#endif
