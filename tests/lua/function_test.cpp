#include "function_test.h"

#include "lua/function.h"
#include "lua/table.h"

void FunctionTest::init(void) {
    QVERIFY(this->lua.init());
}

void FunctionTest::c_fn(void) {
    constexpr auto f = [](nngn::lua::state_view l) {
        l.globals()["g"] = true;
        return 42;
    };
    nngn::lua::function_value{this->lua.push(f)}();
    QCOMPARE(this->lua.top(), 1);
    QCOMPARE(this->lua.get(-1), 42);
    QCOMPARE(this->lua.globals()["g"], true);
}

void FunctionTest::lua_fn(void) {
    this->lua.dostring("function f(i) return i + 1 end");
    nngn::lua::function_value{this->lua.globals()["f"]}(42);
    QCOMPARE(this->lua.top(), 1);
    QCOMPARE(this->lua.get(-1), 43);
}

void FunctionTest::c_lua_fn(void) {
    constexpr auto f = [](int i) { return i + 1; };
    this->lua.globals()["f"] = f;
    nngn::lua::function_value{this->lua.globals()["f"]}(42);
    QCOMPARE(this->lua.top(), 1);
    QCOMPARE(this->lua.get(-1), 43);
}

void FunctionTest::call(void) {
    constexpr auto f = [](lua_Integer i, lua_Number n) {
        return static_cast<lua_Number>(i) + n;
    };
    this->lua.push(nngn::lua::nil);
    this->lua.push(42);
    this->lua.push(43.0);
    this->lua.push(nngn::lua::call(this->lua, f, 2));
    QCOMPARE(this->lua.get(-1), 85.0);
}

void FunctionTest::state(void) {
    this->lua.call([](nngn::lua::state_view l) { l.globals()["g"] = true; });
    QCOMPARE(this->lua.globals()["g"], true);
}

void FunctionTest::pcall(void) {
    const auto h = this->lua.push(nngn::lua::msgh);
    constexpr auto f_ok = []{};
    QCOMPARE(this->lua.pcall(h, f_ok), LUA_OK);
    constexpr auto f_err = [] { return nngn::lua::error{"error"}; };
    QCOMPARE(this->lua.pcall(h, f_err), LUA_ERRRUN);
}

QTEST_MAIN(FunctionTest)
