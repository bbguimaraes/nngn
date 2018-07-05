#include <chrono>

#include "font/textbox.h"
#include "timing/timing.h"

#include "textbox_test.h"

using namespace std::chrono_literals;

void TextboxTest::constructor() {
    nngn::Textbox();
}

void TextboxTest::update() {
    const auto dt = [](auto t) {
        using T = decltype(nngn::Textbox::timer);
        return std::chrono::duration_cast<T>(t).count();
    };
    nngn::Timing t;
    nngn::Fonts fonts;
    nngn::Font font;
    font.size = 32;
    fonts.add(font);
    nngn::Textbox b;
    b.init(&fonts);
    b.speed = 100ms;
    b.str = nngn::Text(font, "0123456789", 0);
    t.dt = 100ms;
    b.update(t);
    QCOMPARE(b.timer.count(), dt(0ms));
    QCOMPARE(b.str.cur, 1ul);
    t.dt = 50ms;
    b.update(t);
    QCOMPARE(b.timer.count(), dt(50ms));
    QCOMPARE(b.str.cur, 1ul);
    t.dt = 250ms;
    b.update(t);
    QCOMPARE(b.timer.count(), dt(0ms));
    QCOMPARE(b.str.cur, 4ul);
    b.speed = 1ms;
    t.dt = 100ms;
    b.update(t);
    QCOMPARE(b.timer.count(), dt(94ms));
    QCOMPARE(b.str.cur, 10ul);
}

QTEST_MAIN(TextboxTest)
