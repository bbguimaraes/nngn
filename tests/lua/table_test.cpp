#include "table_test.h"

#include "lua/state.h"
#include "lua/utils.h"

void TableTest::global(void) {
    nngn::lua::state L = {};
    QVERIFY(L.init());
    lua_pushinteger(L, 42);
    lua_setglobal(L, "g");
    const auto gt = nngn::lua::global_table{L};
    lua_Integer g = gt["g"];
    QCOMPARE(g, 42);
    gt["g"] = 43;
    g = gt["g"];
    QCOMPARE(g, 43);
}

void TableTest::proxy(void) {
    nngn::lua::state L = {};
    QVERIFY(L.init());
    lua_newtable(L);
    lua_pushinteger(L, 42);
    lua_setfield(L, -2, "f");
    const auto t = nngn::lua::table{L, 1};
    lua_Integer f = t["f"];
    QCOMPARE(f, 42);
    t["f"] = 43;
    f = t["f"];
    QCOMPARE(f, 43);
}

void TableTest::proxy_multi(void) {
    nngn::lua::state L = {};
    QVERIFY(L.init());
    lua_newtable(L);
    lua_newtable(L);
    lua_newtable(L);
    lua_newtable(L);
    lua_pushinteger(L, 42);
    lua_setfield(L, -2, "f");
    lua_setfield(L, -2, "t2");
    lua_setfield(L, -2, "t1");
    lua_setfield(L, -2, "t0");
    const auto t = nngn::lua::table{L, 1};
    lua_Integer f = t["t0"]["t1"]["t2"]["f"];
    QCOMPARE(f, 42);
    t["t0"]["t1"]["t2"]["f"] = 43;
    f = t["t0"]["t1"]["t2"]["f"];
    QCOMPARE(f, 43);
}

void TableTest::user_type(void) {
    nngn::lua::state L = {};
    QVERIFY(L.init());
    struct S {
        int i;
        int f(void) const { return this->i + 1; }
    } s = {.i = 42};
    const auto gt = nngn::lua::global_table(L);
    const auto m = gt.new_user_type<S>("S");
    m["i"] = nngn::lua::readonly(&S::i);
    m["f"] = &S::f;
    gt["s"] = &s;
    S *const p = gt["s"];
    QCOMPARE(p, &s);
    QVERIFY(L.dostring("i = s.i"));
    QCOMPARE(static_cast<lua_Integer>(gt["i"]), s.i);
    QVERIFY(L.dostring("i = s:f()"));
    QCOMPARE(gt["i"].get<int>(), s.f());
}

QTEST_MAIN(TableTest)
