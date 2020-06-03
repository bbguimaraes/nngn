#include <ElysianLua/elysian_lua_function.hpp>
#include <ElysianLua/elysian_lua_table.hpp>
#include <ElysianLua/elysian_lua_thread_view.hpp>

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
    using elysian::lua::LuaPair;
    using elysian::lua::LuaTableValues;
    elysian::lua::LuaVM::initialize(this->lua.L);
    const elysian::lua::ThreadView el(this->lua.L);
    this->sprite.load(el.createTable(elysian::lua::LuaTableValues{
        LuaPair{1, 32},
        LuaPair{2, 64},
        LuaPair{3, LuaTableValues{
            LuaPair{1, LuaTableValues{
                LuaPair{1, LuaTableValues{
                    LuaPair{1, 0},
                    LuaPair{2, 1},
                    LuaPair{3, 2}}},
                LuaPair{2, LuaTableValues{
                    LuaPair{1, 2},
                    LuaPair{2, 3},
                    LuaPair{3, 4}}}}},
            LuaPair{2, LuaTableValues{
                LuaPair{1,
                    LuaTableValues{
                        LuaPair{1, 5},
                        LuaPair{2, 6},
                        LuaPair{3, 7},
                        LuaPair{4, 8},
                        LuaPair{5, 9}}},
                LuaPair{2,
                    LuaTableValues{
                        LuaPair{1, 10},
                        LuaPair{2, 11},
                        LuaPair{3, 12},
                        LuaPair{4, 13},
                        LuaPair{5, 14}}}}},
            LuaPair{3, LuaTableValues{
                LuaPair{1,
                    LuaTableValues{
                        LuaPair{1, 15},
                        LuaPair{2, 16},
                        LuaPair{3, 17},
                        LuaPair{4, 18},
                        LuaPair{5, 0}}}}}}}}));
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
