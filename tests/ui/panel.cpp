#include "panel.hpp"

#include <array>
#include <string_view>

#include "ui/panel.hpp"

namespace {

template<std::size_t R, std::size_t C>
void add_commands(
    impero::Panel *p,
    const std::array<std::string_view, R * C> &a
) {
    for(std::size_t x = 0; x < C; ++x)
        for(std::size_t y = 0; y < R; ++y)
            p->add_command(a[C * y + x], y, x);
}

}

void PanelTest::move_selection(void) {
    using namespace std::literals;
    constexpr std::size_t N = 3;
    impero::Panel p;
    add_commands<N, N>(&p, {
        "cmd0"sv, "cmd3"sv, "cmd6"sv,
        "cmd1"sv, "cmd4"sv, "cmd7"sv,
        "cmd2"sv, "cmd5"sv, "cmd8"sv});
    std::size_t selection = {};
    QObject::connect(
        &p, &impero::Panel::command_selected,
        [&selection](std::size_t i) { selection = i; });
    for(std::size_t i = 0, n = N * N; i < n; ++i) {
        p.select_command();
        QCOMPARE(selection, i);
        p.move_selection(true);
    }
    p.select_command();
    QCOMPARE(selection, std::size_t{});
    p.move_selection(false);
    for(std::size_t i = N * N; --i;) {
        p.select_command();
        QCOMPARE(selection, i);
        p.move_selection(false);
    }
    p.select_command();
    QCOMPARE(selection, std::size_t{});
}

void PanelTest::update_filter(void) {
    using namespace std::literals;
    constexpr std::size_t N = 3;
    impero::Panel p;
    add_commands<N, N>(&p, {
        "xxxxxxxxxxxxxx", "xxxxxxxxxxxxxx", "xxxxxxxxxxxxxx",
        "xxx_filter_aaa", "xxx_filter_ccc", "xxx_filter_ddd",
        "xxx_filter_bbb", "xxxxxxxxxxxxxx", "xxx_filter_eee"});
    std::size_t selection = {};
    QObject::connect(
        &p, &impero::Panel::command_selected,
        [&selection](std::size_t i) { selection = i; });
    p.update_filter("filter");
    for(auto i : {1, 2, 4, 7, 8}) {
        p.select_command();
        QCOMPARE(selection, static_cast<std::size_t>(i));
        p.move_selection(true);
    }
    p.update_filter("");
    p.select_command();
    QCOMPARE(selection, std::size_t{1});
    p.update_filter("no match");
    QCOMPARE(selection, std::size_t{1});
    p.move_selection(true);
    QCOMPARE(selection, std::size_t{1});
    p.move_selection(false);
    QCOMPARE(selection, std::size_t{1});
    p.update_filter("");
    QCOMPARE(selection, std::size_t{1});
}

QTEST_MAIN(PanelTest)
