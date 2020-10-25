#ifndef IMPERO_UI_UTILS_H
#define IMPERO_UI_UTILS_H

#include <functional>
#include <numeric>
#include <ranges>
#include <utility>

#include <QtWidgets/QLayout>

namespace impero {

template<typename T>
inline constexpr auto as = std::views::transform(
    []<typename F>(F f) { return static_cast<T>(f); });

template<typename R, typename I = std::ranges::range_value_t<R>, typename ...Ts>
inline decltype(auto) accumulate(R &&r, I &&i = {}, Ts &&...ts) {
    return std::accumulate(
        begin(r), end(r), std::forward<I>(i), std::forward<Ts>(ts)...);
}

inline auto layout_view(QLayout *l) {
    return std::ranges::iota_view(0, l->count())
        | std::views::transform(std::bind_front(&QLayout::itemAt, l));
}

inline auto layout_widget_view(QLayout *l) {
    return layout_view(l)
        | std::views::transform(&QLayoutItem::widget)
        | std::views::filter(std::identity{});
}

}

#endif
