#include "register_test.h"

#include "utils/regexp.h"

#include "lua/function.h"
#include "lua/register.h"
#include "lua/state.h"
#include "lua/table.h"

#include "tests/tests.h"

namespace {

struct member {};

struct user_type {
    int i;
    member m;
    int f(void) const { return this->i; }
};

using T = user_type;

}

NNGN_LUA_DECLARE_USER_TYPE(member)
NNGN_LUA_DECLARE_USER_TYPE(user_type)

void RegisterTest::init(void) {
    QVERIFY(this->lua.init());
    this->g = lua.globals();
    this->mt = lua.new_user_type<T>();
    QCOMPARE(this->lua.top(), 1);
}

void RegisterTest::cleanup(void) {
    this->mt.release();
}

void RegisterTest::user_type(void) {
    const auto v = this->lua.push(T{});
    const auto i = v.index();
    QCOMPARE(this->lua.top(), 2);
    QCOMPARE(v.get_type(), nngn::lua::type::user_data);
    QVERIFY(!lua_islightuserdata(this->lua, i));
    QVERIFY(nngn::lua::user_data<T>::check_type(this->lua, i));
}

void RegisterTest::meta_table(void) {
    const auto v = this->lua.push(T{});
    const auto i = v.index();
    QCOMPARE(this->lua.top(), 2);
    lua_getmetatable(this->lua, i);
    QVERIFY(lua_compare(this->lua, this->mt.index(), -1, LUA_OPEQ));
    const auto s = nngn::regexp_replace(
        this->lua.to_string(i).second, std::regex{"0x[0-9a-f]+"}, "0x0");
    QCOMPARE(s, "user_type: 0x0");
}

void RegisterTest::accessor(void) {
    this->lua.new_user_type<member>();
    this->mt["m"] = nngn::lua::accessor<&user_type::m>;
    T u = {};
    this->g["u"] = &u;
    QVERIFY(this->lua.dostring("m = u:m()"));
    QCOMPARE(this->g["m"], &u.m);
}

void RegisterTest::value_accessor(void) {
    this->mt["i"] = nngn::lua::value_accessor<&user_type::i>;
    T u = {.i = 42};
    this->g["u"] = &u;
    QVERIFY(this->lua.dostring("i = u:i()"));
    QCOMPARE(this->g["i"], u.i);
}

void RegisterTest::member_fn(void) {
    this->mt["f"] = &user_type::f;
    T u = {.i = 42};
    this->g["u"] = &u;
    QVERIFY(this->lua.dostring("i = u:f()"));
    const int ret = this->g["i"];
    QCOMPARE(ret, u.i);
}

QTEST_MAIN(RegisterTest)
