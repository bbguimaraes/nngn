#include <sol/state_view.hpp>

#include "luastate.h"

#include "luastate_test.h"

struct RegisterAllFixture {};

NNGN_LUA_PROXY(RegisterAllFixture, sol::no_constructor)

void LuastateTest::luastate() {
    LuaState lua;
    QCOMPARE(lua.L, nullptr);
}

void LuastateTest::init_state() {
    LuaState lua;
    lua.init();
    QVERIFY(lua.L != nullptr);
}

void LuastateTest::register_all() {
    LuaState lua;
    lua.init();
    NNGN_LUA_REGISTER_RegisterAllFixture_F(lua.L);
    lua.register_all();
    luaL_dostring(lua.L, "return RegisterAllFixture ~= nil");
    QCOMPARE(lua_toboolean(lua.L, -1), 1);
}

void LuastateTest::traceback() {
    LuaState lua;
    lua.init();
    QVERIFY(lua.dostring(
        "function a(s) b(s) end\n"
        "function b(s) c(s) end\n"
        "function c(s) d(s) end\n"));
    std::string tb;
    lua_pushcfunction(lua.L, [](lua_State *L) {
        *static_cast<std::string*>(lua_touserdata(L, -1)) =
            LuaState::Traceback(L).str();
        return 0;
    });
    lua_setglobal(lua.L, "d");
    lua_getglobal(lua.L, "a");
    lua_pushlightuserdata(lua.L, &tb);
    if(lua_pcall(lua.L, 1, 0, 0) != LUA_OK)
        QFAIL(lua_tostring(lua.L, -1));
    QCOMPARE(lua_gettop(lua.L), 0);
    constexpr const char *v =
        "stack traceback:\n"
        "\t[C]: in function 'd'\n"
        "\t[string \"function a(s) b(s) end...\"]:3: in function 'c'\n"
        "\t[string \"function a(s) b(s) end...\"]:2: in function 'b'\n"
        "\t[string \"function a(s) b(s) end...\"]:1: in function 'a'";
    QCOMPARE(tb.c_str(), v);
}

QTEST_MAIN(LuastateTest)
