#include "input_test.h"

#include <sstream>
#include <string_view>

#include "input/group.h"
#include "input/input.h"
#include "lua/state.h"

#include "tests/tests.h"

using nngn::BindingGroup;
using nngn::Input;

Q_DECLARE_METATYPE(std::string_view)
Q_DECLARE_METATYPE(Input::Selector)
Q_DECLARE_METATYPE(Input::Action)
Q_DECLARE_METATYPE(Input::Modifier)

namespace {

constexpr int KEY = 1;
constexpr auto *TEST_FUNC = "return function(...) key, press, mods = ... end";

constexpr auto PRESS = Input::Selector::PRESS;
constexpr auto CTRL = Input::Selector::CTRL;
constexpr auto ALT = Input::Selector::ALT;
constexpr auto PRESS_CTRL = static_cast<Input::Selector>(PRESS | CTRL);
constexpr auto PRESS_ALT = static_cast<Input::Selector>(PRESS | ALT);
constexpr auto CTRL_ALT = static_cast<Input::Selector>(CTRL | ALT);
constexpr auto PRESS_CTRL_ALT =
    static_cast<Input::Selector>(PRESS | CTRL | ALT);

void register_callback(lua_State *L, Input *i, const char *f) {
    luaL_dostring(L, f);
    i->register_callback();
    lua_pop(L, 1);
}

void register_callback(
    lua_State *L, BindingGroup *g, int key, Input::Selector s
) {
    luaL_dostring(L, TEST_FUNC);
    g->add(L, key, s);
    lua_pop(L, 1);
}

QString result_str(int key, bool press, int mods)
    { return QString::asprintf( "%d %d %d", key, press, mods); }

QString result_str(lua_State *L) {
    lua_getglobal(L, "key");
    lua_getglobal(L, "press");
    lua_getglobal(L, "mods");
    const auto ret = result_str(
        static_cast<int>(lua_tointeger(L, -3)),
        lua_toboolean(L, -2),
        static_cast<int>(lua_tointeger(L, -1)));
    lua_pop(L, 3);
    return ret;
}

const QString not_called = result_str(0, 0, 0);

}

void InputTest::get_keys() {
    constexpr std::int32_t key = 'K';
    std::int32_t v = key;
    Input input;
    input.get_keys(1, &v);
    QVERIFY(!v);
    struct : Input::Source {
        void get_keys(std::size_t, std::int32_t *p) const override { *p = 1; }
    } s;
    input.add_source(std::make_unique<decltype(s)>(s));
    v = key;
    input.get_keys(1, &v);
    QCOMPARE(v, 1);
}

void InputTest::get_keys_override() {
    constexpr std::int32_t key = 'K';
    std::int32_t v = key;
    Input input;
    input.override_keys(true, 1, &v);
    struct : Input::Source {
        void get_keys(std::size_t, std::int32_t*) const override {}
    } s;
    input.add_source(std::make_unique<decltype(s)>(s));
    input.get_keys(1, &v);
    QCOMPARE(v, 1);
}

void InputTest::override_keys() {
    const auto str = [](const auto &v) {
        std::stringstream s;
        for(auto x : v)
            s << static_cast<bool>(x);
        return s.str();
    };
    constexpr std::array<std::int32_t, 4> keys = {'K', 'E', 'Y', 'S'};
    constexpr auto n = keys.size();
    Input input;
    auto v = keys;
    auto *const p = v.data();
    input.get_keys(n, p);
    QCOMPARE(str(v), "0000");
    v = keys;
    QVERIFY(input.override_keys(true, 1, p));
    QVERIFY(input.override_keys(true, 1, p + 2));
    v = keys;
    input.has_override(n, p);
    QCOMPARE(str(v), "1010");
    v = keys;
    input.get_keys(n, p);
    QCOMPARE(str(v), "1010");
    v = keys;
    QVERIFY(input.override_keys(false, 1, p));
    QVERIFY(input.override_keys(false, 1, p + 2));
    input.get_keys(n, p);
    QCOMPARE(str(v), "0000");
}

void InputTest::register_callback_data() {
    QTest::addColumn<bool>("handles");
    QTest::addColumn<std::string_view>("f");
    QTest::newRow("handles")
        << true << std::string_view(TEST_FUNC);
    QTest::newRow("delegates")
        << true << std::string_view("return function() end");
    QTest::newRow("overrides")
        << false << std::string_view("return function() return true end");
}

void InputTest::register_callback() {
    QFETCH(const bool, handles);
    QFETCH(const std::string_view, f);
    nngn::lua::state lua = {};
    QVERIFY(lua.init());
    Input input;
    BindingGroup group;
    input.init(lua);
    input.set_binding_group(&group);
    ::register_callback(lua, &input, f.data());
    ::register_callback(lua, &group, KEY, {});
    QVERIFY(input.key_callback(KEY, Input::KEY_PRESS, {}));
    const auto result = handles
        ? result_str(KEY, true, 0)
        : not_called;
    QCOMPARE(result_str(lua), result);
}

void InputTest::register_binding_wrong_key_data() {
    QTest::addColumn<Input::Selector>("selector");
    QTest::newRow("s:    ") << Input::Selector{};
    QTest::newRow("s: p  ") << PRESS;
    QTest::newRow("s:  c ") << CTRL;
    QTest::newRow("s:   a") << ALT;
    QTest::newRow("s: pc ") << PRESS_CTRL;
    QTest::newRow("s:  ca") << CTRL_ALT;
    QTest::newRow("s: p a") << PRESS_ALT;
    QTest::newRow("s: pca") << PRESS_CTRL_ALT;
}

void InputTest::register_binding_wrong_key() {
    QFETCH(const Input::Selector, selector);
    nngn::lua::state lua = {};
    QVERIFY(lua.init());
    Input input;
    BindingGroup group;
    input.init(lua);
    input.set_binding_group(&group);
    ::register_callback(lua, &group, KEY, selector);
    QVERIFY(input.key_callback(0, {}, {}));
    QCOMPARE(result_str(lua), not_called);
}

void InputTest::register_binding_data() {
    constexpr auto NS = Input::Selector{}, P = PRESS, C = CTRL, A = ALT;
    constexpr auto PC = PRESS_CTRL, PA = PRESS_ALT, CA = CTRL_ALT;
    constexpr auto PCA = PRESS_CTRL_ALT;
    constexpr auto KP = Input::KEY_PRESS;
    constexpr auto KR = Input::KEY_RELEASE;
    constexpr auto KC = Input::MOD_CTRL;
    constexpr auto KA = Input::MOD_ALT;
    constexpr auto KCA = static_cast<Input::Modifier>(KC | KA);
    QTest::addColumn<bool>("matches");
    QTest::addColumn<Input::Selector>("selector");
    QTest::addColumn<Input::Action>("action");
    QTest::addColumn<Input::Modifier>("mods");
    QTest::newRow("s:      e:    ") << true  << NS  << KR << Input::Modifier{};
    QTest::newRow("s: p    e:    ") << false << P   << KR << Input::Modifier{};
    QTest::newRow("s:  c   e:    ") << false << C   << KR << Input::Modifier{};
    QTest::newRow("s:   a  e:    ") << false << A   << KR << Input::Modifier{};
    QTest::newRow("s: pc   e:    ") << false << PC  << KR << Input::Modifier{};
    QTest::newRow("s:  ca  e:    ") << false << CA  << KR << Input::Modifier{};
    QTest::newRow("s: p a  e:    ") << false << PA  << KR << Input::Modifier{};
    QTest::newRow("s: pca  e:    ") << false << PCA << KR << Input::Modifier{};
    QTest::newRow("s:      e: p  ") << true  << NS  << KP << Input::Modifier{};
    QTest::newRow("s: p    e: p  ") << true  << P   << KP << Input::Modifier{};
    QTest::newRow("s:  c   e: p  ") << false << C   << KP << Input::Modifier{};
    QTest::newRow("s:   a  e: p  ") << false << A   << KP << Input::Modifier{};
    QTest::newRow("s: pc   e: p  ") << false << PC  << KP << Input::Modifier{};
    QTest::newRow("s:  ca  e: p  ") << false << CA  << KP << Input::Modifier{};
    QTest::newRow("s: p a  e: p  ") << false << PA  << KP << Input::Modifier{};
    QTest::newRow("s: pca  e: p  ") << false << PCA << KP << Input::Modifier{};
    QTest::newRow("s:      e:  c ") << true  << NS  << KR << KC;
    QTest::newRow("s: p    e:  c ") << false << P   << KR << KC;
    QTest::newRow("s:  c   e:  c ") << true  << C   << KR << KC;
    QTest::newRow("s:   a  e:  c ") << false << A   << KR << KC;
    QTest::newRow("s: pc   e:  c ") << false << PC  << KR << KC;
    QTest::newRow("s:  ca  e:  c ") << false << CA  << KR << KC;
    QTest::newRow("s: p a  e:  c ") << false << PA  << KR << KC;
    QTest::newRow("s: pca  e:  c ") << false << PCA << KR << KC;
    QTest::newRow("s:      e:   a") << true  << NS  << KR << KA;
    QTest::newRow("s: p    e:   a") << false << P   << KR << KA;
    QTest::newRow("s:  c   e:   a") << false << C   << KR << KA;
    QTest::newRow("s:   a  e:   a") << true  << A   << KR << KA;
    QTest::newRow("s: pc   e:   a") << false << PC  << KR << KA;
    QTest::newRow("s:  ca  e:   a") << false << CA  << KR << KA;
    QTest::newRow("s: p a  e:   a") << false << PA  << KR << KA;
    QTest::newRow("s: pca  e:   a") << false << PCA << KR << KA;
    QTest::newRow("s:      e: pc ") << true  << NS  << KP << KC;
    QTest::newRow("s: p    e: pc ") << true  << P   << KP << KC;
    QTest::newRow("s:  c   e: pc ") << true  << C   << KP << KC;
    QTest::newRow("s:   a  e: pc ") << false << A   << KP << KC;
    QTest::newRow("s: pc   e: pc ") << true  << PC  << KP << KC;
    QTest::newRow("s:  ca  e: pc ") << false << CA  << KP << KC;
    QTest::newRow("s: p a  e: pc ") << false << PA  << KP << KC;
    QTest::newRow("s: pca  e: pc ") << false << PCA << KP << KC;
    QTest::newRow("s:      e:  ca") << true  << NS  << KR << KCA;
    QTest::newRow("s: p    e:  ca") << false << P   << KR << KCA;
    QTest::newRow("s:  c   e:  ca") << true  << C   << KR << KCA;
    QTest::newRow("s:   a  e:  ca") << true  << A   << KR << KCA;
    QTest::newRow("s: pc   e:  ca") << false << PC  << KR << KCA;
    QTest::newRow("s:  ca  e:  ca") << true  << CA  << KR << KCA;
    QTest::newRow("s: p a  e:  ca") << false << PA  << KR << KCA;
    QTest::newRow("s: pca  e:  ca") << false << PCA << KR << KCA;
    QTest::newRow("s:      e: p a") << true  << NS  << KP << KA;
    QTest::newRow("s: p    e: p a") << true  << P   << KP << KA;
    QTest::newRow("s:  c   e: p a") << false << C   << KP << KA;
    QTest::newRow("s:   a  e: p a") << true  << A   << KP << KA;
    QTest::newRow("s: pc   e: p a") << false << PC  << KP << KA;
    QTest::newRow("s:  ca  e: p a") << false << CA  << KP << KA;
    QTest::newRow("s: p a  e: p a") << true  << PA  << KP << KA;
    QTest::newRow("s: pca  e: p a") << false << PCA << KP << KA;
    QTest::newRow("s:      e: pca") << true  << NS  << KP << KCA;
    QTest::newRow("s: p    e: pca") << true  << P   << KP << KCA;
    QTest::newRow("s:  c   e: pca") << true  << C   << KP << KCA;
    QTest::newRow("s:   a  e: pca") << true  << A   << KP << KCA;
    QTest::newRow("s: pc   e: pca") << true  << PC  << KP << KCA;
    QTest::newRow("s:  ca  e: pca") << true  << CA  << KP << KCA;
    QTest::newRow("s: p a  e: pca") << true  << PA  << KP << KCA;
    QTest::newRow("s: pca  e: pca") << true  << PCA << KP << KCA;
}

void InputTest::register_binding() {
    QFETCH(const bool, matches);
    QFETCH(const Input::Selector, selector);
    QFETCH(const Input::Action, action);
    QFETCH(const Input::Modifier, mods);
    nngn::lua::state lua = {};
    QVERIFY(lua.init());
    Input input;
    BindingGroup group;
    input.init(lua);
    input.set_binding_group(&group);
    ::register_callback(lua, &group, KEY, selector);
    QVERIFY(input.key_callback(KEY, action, mods));
    const auto result = matches
        ? result_str(KEY, action == Input::KEY_PRESS, mods)
        : not_called;
    QCOMPARE(result_str(lua), result);
}

QTEST_MAIN(InputTest)
