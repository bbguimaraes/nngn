#ifndef NNGN_UTILS_VECTOR_H
#define NNGN_UTILS_VECTOR_H

#include <algorithm>
#include <cstring>
#include <iterator>
#include <numeric>
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

template<typename T, std::size_t N>
consteval auto to_array(const std::span<T, N> s)
    { return to_array<N>(std::data(s)); }

template<typename T, std::size_t N>
constexpr bool is_adjacent(const std::span<T, N> s) {
    constexpr auto not_adj = [](const auto &lhs, const auto &rhs) {
        return lhs + T{1} != rhs;
    };
    using std::end;
    return std::adjacent_find(begin(s), end(s), not_adj) == end(s);
}

template<typename T>
auto reduce(std::span<T> s) {
    using std::begin, std::end;
    return std::reduce(begin(s), end(s));
}

template<typename I, typename O>
void fill_with_pattern(I f, I l, O df, O dl) {
    using IT = std::iterator_traits<I>;
    using OT = std::iterator_traits<O>;
    static_assert(
        std::is_base_of_v<
            std::input_iterator_tag,
            typename IT::iterator_category>);
//    static_assert(
//        std::is_base_of_v<
//            std::output_iterator_tag,
//            typename OT::iterator_category>);
    static_assert(
        std::is_convertible_v<
            typename IT::value_type,
            typename OT::value_type>);
    std::generate(df, dl, [i = f, f, l]() mutable {
        if(i == l)
            i = f;
        return *i++;
    });
}

template<typename T, std::size_t N>
constexpr void memcpy(void *dst, std::span<T, N> s)
    { std::memcpy(dst, data(s), s.size_bytes()); }

}

#endif
