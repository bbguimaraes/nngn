#include "table_test.h"

#include "lua/function.h"
#include "lua/iter.h"
#include "lua/push.h"
#include "lua/register.h"
#include "lua/table.h"
#include "lua/utils.h"

#include "tests/tests.h"

using namespace std::string_view_literals;

struct test_struct {
    int i;
    int f(void) const { return this->i + 1; }
};

NNGN_LUA_DECLARE_USER_TYPE(test_struct)

void TableTest::init(void) {
    QVERIFY(this->lua.init());
}

void TableTest::global(void) {
    const nngn::lua::global_table g = this->lua.globals();
    g["g"] = 42;
    lua_Integer i = g["g"];
    QCOMPARE(i, 42);
    g["g"] = 43;
    i = g["g"];
    QCOMPARE(i, 43);
}

void TableTest::proxy(void) {
    const auto t = this->lua.create_table();
    t["f"] = 42;
    lua_Integer i = t["f"];
    QCOMPARE(i, 42);
    t["f"] = 43;
    i = t["f"];
    QCOMPARE(i, 43);
}

void TableTest::proxy_multi(void) {
    lua_newtable(this->lua);
    lua_newtable(this->lua);
    lua_newtable(this->lua);
    lua_newtable(this->lua);
    lua_newtable(this->lua);
    lua_pushinteger(this->lua, 42);
    lua_setfield(this->lua, -2, "i");
    lua_setfield(this->lua, -2, "f");
    lua_setfield(this->lua, -2, "t2");
    lua_setfield(this->lua, -2, "t1");
    lua_setfield(this->lua, -2, "t0");
    QCOMPARE(this->lua.top(), 1);
    const auto t = nngn::lua::table{this->lua, 1};
    const nngn::lua::table f0 = t["t0"]["t1"]["t2"]["f"];
    QCOMPARE(this->lua.top(), 2);
    QCOMPARE(f0["i"], 42);
    t["t0"]["t1"]["t2"]["f"]["i"] = 43;
    QCOMPARE(this->lua.top(), 2);
    const nngn::lua::table f1 = t["t0"]["t1"]["t2"]["f"];
    QCOMPARE(this->lua.top(), 3);
    QCOMPARE(f1["i"], 43);
}

void TableTest::proxy_default(void) {
    const auto t = this->lua.create_table();
    t["x"] = 42;
    const auto x = t["x"].get<lua_Integer>();
    QCOMPARE(x, 42);
    const auto y = t["y"].get<lua_Integer>(43);
    QCOMPARE(y, 43);
    QCOMPARE(this->lua.top(), 1);
}

void TableTest::proxy_default_table(void) {
    const auto t = this->lua.create_table();
    t["x"] = this->lua.create_table();
    const auto x = t["x"].get<nngn::lua::table>();
    QCOMPARE(x.get_type(), nngn::lua::type::table);
    const auto y = t["y"].get<nngn::lua::table>();
    QCOMPARE(y.state(), nullptr);
    QCOMPARE(this->lua.top(), 3);
}

void TableTest::user_type(void) {
    test_struct s = {.i = 42};
    const auto g = this->lua.globals();
    const auto m = this->lua.create_table();
    m["__index"] = m;
    m["i"] = nngn::lua::value_accessor<&test_struct::i>;
    m["f"] = &test_struct::f;
    g["test_struct"] = m;
    g["s"] = &s;
    test_struct *const p = g["s"];
    QCOMPARE(p, &s);
    QVERIFY(this->lua.dostring("i = s:i()"));
    QCOMPARE((g["i"]), s.i);
    QVERIFY(this->lua.dostring("i = s:f()"));
    QCOMPARE(g["i"], s.f());
}

void TableTest::size(void) {
    const auto t = this->lua.create_table();
    QCOMPARE(t.size(), 0);
    t[1] = 1;
    t[2] = 2;
    t[3] = 3;
    QCOMPARE(t.size(), 3);
    t[3] = nngn::lua::nil;
    QCOMPARE(t.size(), 2);
    t[1] = nngn::lua::nil;
    t[2] = nngn::lua::nil;
    QCOMPARE(t.size(), 0);
    t["a"] = 1;
    QCOMPARE(t.size(), 0);
}

void TableTest::iter(void) {
    using V = std::vector<std::pair<std::string, lua_Integer>>;
    const V expected = {{"a", 0}, {"b", 1}, {"c", 2}};
    V ret = {};
    {
        const auto t = this->lua.create_table();
        for(const auto &[k, v] : expected)
            t[k] = v;
        for(auto [k, v] : t)
            ret.emplace_back(k, v);
    }
    QCOMPARE(this->lua.top(), 0);
    std::ranges::sort(ret);
    QCOMPARE(ret, expected);
}

void TableTest::iter_ipairs(void) {
    using V = std::vector<std::pair<lua_Integer, std::string>>;
    const V expected = {{1, "a"}, {2, "b"}, {3, "c"}};
    V ret = {};
    {
        const auto t = this->lua.create_table();
        for(const auto &[i, x] : expected)
            t[i] = x;
        for(auto [i, x] : ipairs(t))
            ret.emplace_back(i, x);
    }
    QCOMPARE(this->lua.top(), 0);
    QCOMPARE(ret, expected);
}

void TableTest::iter_empty(void) {
    const auto t = this->lua.create_table();
    for([[maybe_unused]] auto x : t)
        ;
    QCOMPARE(this->lua.top(), 1);
}

void TableTest::iter_ipairs_empty(void) {
    const auto t = this->lua.create_table();
    for([[maybe_unused]] auto x : ipairs(t))
        ;
    QCOMPARE(this->lua.top(), 1);
}

void TableTest::iter_stack(void) {
    const auto t = this->lua.create_table();
    t["a"] = 1;
    for(auto b = t.begin(), e = t.end(); b != e; ++b) {
        NNGN_ANON_DECL(this->lua.push<nngn::lua::value>(nngn::lua::nil));
        const auto [k, v] = *b;
        QCOMPARE(k, "a"sv);
        QCOMPARE(v, 1);
    }
    QCOMPARE(this->lua.top(), 1);
}

void TableTest::iter_ipairs_stack(void) {
    const auto t = this->lua.create_table();
    t[1] = "a";
    for(auto b = t.ibegin(), e = t.iend(); b != e; ++b) {
        NNGN_ANON_DECL(this->lua.push<nngn::lua::value>(nngn::lua::nil));
        const auto [i, x] = *b;
        QCOMPARE(i, 1);
        QCOMPARE(x, "a"sv);
    }
    QCOMPARE(this->lua.top(), 1);
}

void TableTest::iter_nested(void) {
    const auto t = this->lua.create_table(3, 0);
    t[1] = nngn::lua::table_map(this->lua, "a", 1, "b", 2, "c", 3);
    t[2] = nngn::lua::table_map(this->lua, "d", 4, "e", 5, "f", 6);
    t[3] = nngn::lua::table_map(this->lua, "g", 7, "h", 8, "k", 9);
    using V = std::vector<std::vector<std::pair<std::string, lua_Integer>>>;
    auto ret = V(3);
    for(auto [i, x] : ipairs(t))
        for(auto [k, v] : nngn::lua::table_view{x})
            ret[static_cast<std::size_t>(i) - 1].emplace_back(k, v);
    for(auto &x : ret)
        std::ranges::sort(x);
    const V expected = {
        {{"a", 1}, {"b", 2}, {"c", 3}},
        {{"d", 4}, {"e", 5}, {"f", 6}},
        {{"g", 7}, {"h", 8}, {"k", 9}},
    };
    QCOMPARE(ret, expected);
}

QTEST_MAIN(TableTest)
