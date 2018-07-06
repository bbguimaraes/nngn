#include "state_test.h"

#include "lua/state.h"
#include "lua/traceback.h"

using nngn::lua::state;

struct RegisterAllFixture {};

NNGN_LUA_PROXY(RegisterAllFixture)

void StateTest::constructor(void) {
    state lua;
    QCOMPARE(static_cast<lua_State*>(lua), nullptr);
}

void StateTest::init(void) {
    state lua;
    QVERIFY(lua.init());
    QVERIFY(static_cast<lua_State*>(lua) != nullptr);
}

void StateTest::register_all(void) {
    state lua;
    QVERIFY(lua.init());
    nngn::lua::static_register::register_all(lua);
    luaL_dostring(lua, "return RegisterAllFixture ~= nil");
    QCOMPARE(lua_toboolean(lua, -1), 1);
}

void StateTest::traceback(void) {
    state lua;
    QVERIFY(lua.init());
    QVERIFY(lua.dostring(
        "function a(s) b(s) end\n"
        "function b(s) c(s) end\n"
        "function c(s) d(s) end\n"));
    std::string tb;
    lua_pushcfunction(lua, [](lua_State *L) {
        *static_cast<std::string*>(lua_touserdata(L, -1)) =
            nngn::lua::traceback(L).str();
        return 0;
    });
    lua_setglobal(lua, "d");
    lua_getglobal(lua, "a");
    lua_pushlightuserdata(lua, &tb);
    if(lua_pcall(lua, 1, 0, 0) != LUA_OK)
        QFAIL(lua_tostring(lua, -1));
    QCOMPARE(lua_gettop(lua), 0);
    constexpr const char *v =
        "stack traceback:\n"
        "\t[C]: in function 'd'\n"
        "\t[string \"function a(s) b(s) end...\"]:3: in function 'c'\n"
        "\t[string \"function a(s) b(s) end...\"]:2: in function 'b'\n"
        "\t[string \"function a(s) b(s) end...\"]:1: in function 'a'";
    QCOMPARE(tb.c_str(), v);
}

QTEST_MAIN(StateTest)
