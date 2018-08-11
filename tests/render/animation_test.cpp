#include "entity.h"

#include "lua/utils.h"
#include "render/render.h"
#include "timing/timing.h"

#include "../tests.h"

#include "animation_test.h"

using namespace std::chrono_literals;

Q_DECLARE_METATYPE(nngn::vec4)
Q_DECLARE_METATYPE(std::chrono::milliseconds)

void AnimationTest::initTestCase() {
    QVERIFY(this->lua.init());
}

void AnimationTest::init() {
    using namespace nngn::lua;
    this->sprite.load(table_array(
        this->lua, 32, 64,
        table_array(
            this->lua,
            table_array(
                this->lua,
                table_array(this->lua, 0, 1, 2),
                table_array(this->lua, 2, 3, 4)),
            table_array(
                this->lua,
                table_array(this->lua,  5, 6,   7,  8,  9),
                table_array(this->lua, 10, 11, 12, 13, 14)),
            table_array(
                this->lua,
                table_array(this->lua, 15, 16, 17, 18, 0)))));
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
