#include <sol/state_view.hpp>

#include "entity.h"
#include "luastate.h"

#include "render/render.h"
#include "timing/timing.h"

#include "animation_test.h"

using namespace std::chrono_literals;

Q_DECLARE_METATYPE(nngn::vec4)
Q_DECLARE_METATYPE(std::chrono::milliseconds)

void AnimationTest::initTestCase() {
    QVERIFY(this->lua.init());
}

void AnimationTest::init() {
    auto sol = sol::state_view(this->lua.L);
    const auto t = sol.create_table_with(
        1, 32,
        2, 64,
        3, sol.create_table_with(
            1, sol.create_table_with(
                1, sol.create_table_with(1, 0, 2, 1, 3, 2),
                2, sol.create_table_with(1, 2, 2, 3, 3, 4)),
            2, sol.create_table_with(
                1, sol.create_table_with(1,  5, 2,  6, 3,  7, 4,  8, 5,  9),
                2, sol.create_table_with(1, 10, 2, 11, 3, 12, 4, 13, 5, 14)),
            3, sol.create_table_with(
                1, sol.create_table_with(1, 15, 2, 16, 3, 17, 4, 18, 5, 0))));
    this->sprite.load(t);
}

void AnimationTest::load_data() {
    QTest::addColumn<size_t>("track");
    QTest::addColumn<std::chrono::milliseconds>("duration");
    QTest::addColumn<nngn::vec4>("uv");
    QTest::newRow("0, 0")
        << static_cast<size_t>(0) << 0ms
        << nngn::vec4(0, 0.984375f, 0.03125f, 0.96875f);
    QTest::newRow("0, 1")
        << static_cast<size_t>(0) << 2ms
        << nngn::vec4(0.0625f, 0.953125f, 0.09375, 0.9375f);
    QTest::newRow("1, 0")
        << static_cast<size_t>(1) << 0ms
        << nngn::vec4(0.15625f, 0.90625f, 0.21875f, 0.875f);
    QTest::newRow("1, 1")
        << static_cast<size_t>(1) << 9ms
        << nngn::vec4(0.3125f,  0.828125f, 0.375f, 0.796875f);
    QTest::newRow("2, 0")
        << static_cast<size_t>(2) << 0ms
        << nngn::vec4(0.46875f, 0.75f, 0.53125f, 0.71875f);
}

void AnimationTest::load() {
    QFETCH(const size_t, track);
    QFETCH(const nngn::vec4, uv);
    QFETCH(const std::chrono::milliseconds, duration);
    nngn::SpriteRenderer r;
    Entity e;
    e.renderer = &r;
    nngn::Timing t;
    t.dt = duration;
    this->sprite.entity = &e;
    this->sprite.set_track(track);
    this->sprite.update(t);
    auto cur = this->sprite.cur();
    QVERIFY(cur);
    const nngn::vec4 v = {r.uv0, r.uv1};
    QCOMPARE(v, uv);
}

void AnimationTest::update() {
    nngn::SpriteRenderer r;
    Entity e;
    e.renderer = &r;
    nngn::Timing t;
    this->sprite.entity = &e;
    this->sprite.set_track(0);
    auto cur = this->sprite.cur();
    t.dt = 1ms;
    this->sprite.update(t);
    QCOMPARE(this->sprite.cur(), cur);
    t.dt = 2ms;
    this->sprite.update(t);
    QVERIFY(this->sprite.cur() != cur);
    cur = this->sprite.cur();
    this->sprite.update(t);
    QCOMPARE(this->sprite.cur(), cur);
    this->sprite.update(t);
    QVERIFY(this->sprite.cur() != cur);
    this->sprite.set_track(2);
    cur = this->sprite.cur();
    t.dt = 1000ms;
    this->sprite.update(t);
    QCOMPARE(this->sprite.cur(), cur);
}

QTEST_MAIN(AnimationTest)
