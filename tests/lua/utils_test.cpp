#include "utils_test.h"

#include "lua/state.h"
#include "lua/utils.h"

#include "tests/tests.h"

void UtilsTest::defer_pop(void) {
    nngn::lua::state lua = {};
    QVERIFY(lua.init());
    lua_pushnil(lua);
    lua_pushnil(lua);
    lua_pushnil(lua);
    QCOMPARE(lua.top(), 3);
    nngn::lua::defer_pop(lua, 3);
    QCOMPARE(lua.top(), 0);
}

void UtilsTest::stack_mark(void) {
    if constexpr(!nngn::Platform::debug)
        return;
    nngn::lua::state lua = {};
    QVERIFY(lua.init());
    lua_pushnil(lua);
    auto s = nngn::Log::capture([&lua] { nngn::lua::stack_mark(lua); });
    QCOMPARE(s, "");
    lua.pop(1);
    assert(!lua.top());
    s = nngn::Log::capture([&lua] {
        NNGN_ANON_DECL(nngn::lua::stack_mark(lua, 1));
        lua_pushnil(lua);
        lua_pushnil(lua);
    });
    constexpr auto expected = "unexpected stack top: 2 != 1\n";
    QCOMPARE(s, expected);
}

QTEST_MAIN(UtilsTest)
