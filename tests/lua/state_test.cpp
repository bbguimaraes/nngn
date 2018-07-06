#include "state_test.h"

#include "lua/function.h"
#include "lua/register.h"
#include "lua/table.h"
#include "lua/traceback.h"

namespace {

struct S {};
void register_S(nngn::lua::table_view) {}

}

NNGN_LUA_DECLARE_USER_TYPE(S)
NNGN_LUA_PROXY(S, register_S)

void StateTest::init(void) {
    QVERIFY(this->lua.init());
    QVERIFY(static_cast<lua_State*>(this->lua) != nullptr);
}

void StateTest::register_all(void) {
    nngn::lua::static_register::register_all(this->lua);
    QCOMPARE(this->lua.globals()["S"], true);
    (void)nngn::lua::is_user_type<S>;
}

void StateTest::traceback(void) {
    QVERIFY(this->lua.dostring(
        "function a(s) b(s) end\n"
        "function b(s) c(s) end\n"
        "function c(s) d(s) end\n"));
    std::string tb = {};
    const auto g = lua.globals();
    g["d"] = [](lua_State *L) {
        *static_cast<std::string*>(lua_touserdata(L, -1)) =
            nngn::lua::traceback(L).str();
        return 0;
    };
    lua.push(g["a"]);
    lua.push(static_cast<void*>(&tb));
    if(lua_pcall(this->lua, 1, 0, 0) != LUA_OK)
        QFAIL(lua_tostring(this->lua, -1));
    QCOMPARE(lua.top(), 0);
    constexpr const char *v =
        "stack traceback:\n"
        "\t[C]: in function 'd'\n"
        "\t[string \"function a(s) b(s) end...\"]:3: in function 'c'\n"
        "\t[string \"function a(s) b(s) end...\"]:2: in function 'b'\n"
        "\t[string \"function a(s) b(s) end...\"]:1: in function 'a'";
    QCOMPARE(tb.c_str(), v);
}

QTEST_MAIN(StateTest)
