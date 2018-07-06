#include "stack_test.h"

#include <cstring>

#include "utils/regexp.h"

#include "lua/function.h"
#include "lua/get.h"
#include "lua/push.h"
#include "lua/register.h"
#include "lua/state.h"
#include "lua/table.h"
#include "lua/user.h"

#include "tests/tests.h"

using namespace std::string_view_literals;

namespace {

struct user_type {};

constexpr std::size_t user_type_alignment = 64;
struct alignas(user_type_alignment) user_type_align {};

struct user_type_destructor {
    NNGN_NO_COPY(user_type_destructor)
    user_type_destructor(void) = default;
    explicit user_type_destructor(bool *p_) : p{p_} {}
    user_type_destructor(user_type_destructor &&rhs) noexcept
        : user_type_destructor{std::exchange(rhs.p, nullptr)} {}
    ~user_type_destructor(void) { if(this->p) *this->p = true; }
    bool *p = nullptr;
};

int test_c_fn(lua_State*) { return 0; }

}

NNGN_LUA_DECLARE_USER_TYPE(user_type)
NNGN_LUA_DECLARE_USER_TYPE(user_type_align)
NNGN_LUA_DECLARE_USER_TYPE(user_type_destructor)

void StackTest::init(void) {
    QVERIFY(this->lua.init());
}

void StackTest::optional(void) {
    constexpr int x = 42;
    this->lua.push(std::optional<int>{});
    QCOMPARE(this->lua.top(), 0);
    this->lua.push(std::optional<int>{x});
    QCOMPARE(this->lua.top(), 1);
    QCOMPARE(this->lua.get<int>(-1), x);
}

void StackTest::boolean(void) {
    constexpr bool x = true;
    this->lua.push(x);
    QCOMPARE(this->lua.top(), 1);
    QCOMPARE(this->lua.get<bool>(-1), x);
}

void StackTest::light_user_data(void) {
    const void *const x = this;
    this->lua.push(x);
    QCOMPARE(this->lua.top(), 1);
    QCOMPARE(this->lua.get<const void*>(-1), x);
    QVERIFY(!lua_islightuserdata(this->lua, -1));
}

void StackTest::integer(void) {
    constexpr int x = 42;
    this->lua.push(x);
    QCOMPARE(this->lua.top(), 1);
    QCOMPARE(this->lua.get<int>(-1), x);
}

void StackTest::number(void) {
    constexpr double x = 42.0;
    this->lua.push(x);
    QCOMPARE(this->lua.top(), 1);
    QCOMPARE(this->lua.get<double>(-1), x);
}

void StackTest::enum_(void) {
    enum class E { x = 42 };
    this->lua.push(E::x);
    QCOMPARE(this->lua.top(), 1);
    QCOMPARE(this->lua.get<E>(-1), E::x);
}

void StackTest::cstr(void) {
    constexpr const char *x = "test";
    this->lua.push(x);
    QCOMPARE(this->lua.top(), 1);
    QCOMPARE(this->lua.get<const char*>(-1), x);
}

void StackTest::string_view(void) {
    constexpr std::string_view x = "test_sv";
    this->lua.push(x);
    QCOMPARE(this->lua.top(), 1);
    QCOMPARE(this->lua.get<std::string_view>(-1), x);
}

void StackTest::string(void) {
    const std::string x = "test_str";
    this->lua.push(x);
    QCOMPARE(this->lua.top(), 1);
    QCOMPARE(this->lua.get<std::string>(-1), x);
}

void StackTest::c_fn(void) {
    this->lua.push(test_c_fn);
    QCOMPARE(this->lua.top(), 1);
    const auto ret = this->lua.get<lua_CFunction>(-1);
    constexpr auto *fp = test_c_fn;
    constexpr auto size = sizeof(lua_CFunction);
    std::array<char, size> f0 = {}, f1 = {};
    std::memcpy(f0.data(), &fp, size);
    std::memcpy(f1.data(), &ret, size);
    QCOMPARE(f0, f1);
}

void StackTest::c_fn_ptr(void) {
    constexpr auto x = +[](lua_State*) -> int { return 0; };
    this->lua.push(x);
    QCOMPARE(this->lua.top(), 1);
    const auto ret = this->lua.get<lua_CFunction>(-1);
    constexpr auto size = sizeof(lua_CFunction);
    std::array<char, size> f0 = {}, f1 = {};
    std::memcpy(f0.data(), &x, size);
    std::memcpy(f1.data(), &ret, size);
    QCOMPARE(f0, f1);
}

void StackTest::lambda(void) {
    constexpr auto x = [](lua_State*) -> int { return 0; };
    this->lua.push(x);
    QCOMPARE(this->lua.top(), 1);
    const auto ret = this->lua.get<lua_CFunction>(-1);
    constexpr auto *fp = +x;
    constexpr auto size = sizeof(lua_CFunction);
    std::array<char, size> f0 = {}, f1 = {};
    std::memcpy(f0.data(), &fp, size);
    std::memcpy(f1.data(), &ret, size);
    QCOMPARE(f0, f1);
}

void StackTest::table(void) {
    this->lua.push(this->lua.create_table().release());
    QCOMPARE(this->lua.top(), 2);
    QCOMPARE(this->lua.get_type(-1), nngn::lua::type::table);
}

void StackTest::optional_table(void) {
    using O = std::optional<nngn::lua::table>;
    QVERIFY(!this->lua.get<O>(1));
    QCOMPARE(this->lua.top(), 0);
    this->lua.push(this->lua.create_table().release());
    QCOMPARE(this->lua.top(), 2);
    QCOMPARE(this->lua.get_type(-1), nngn::lua::type::table);
    const auto o = this->lua.get<O>(2);
    QVERIFY(o);
    QCOMPARE(o->index(), 2);
}

void StackTest::user_type(void) {
    this->lua.new_user_type<::user_type>();
    this->lua.push(::user_type{});
    QCOMPARE(this->lua.top(), 1);
    QCOMPARE(this->lua.get_type(-1), nngn::lua::type::user_data);
    QVERIFY(!lua_islightuserdata(this->lua, -1));
    QVERIFY(nngn::lua::user_data<::user_type>::check_type(this->lua, -1));
    this->lua.destroy();
}

void StackTest::user_type_ptr(void) {
    using T = ::user_type;
    using U = nngn::lua::user_data<T>;
    using UP = nngn::lua::user_data<T*>;
    this->lua.new_user_type<T>();
    this->lua.push(static_cast<T*>(nullptr));
    QCOMPARE(this->lua.top(), 1);
    QCOMPARE(this->lua.get_type(-1), nngn::lua::type::nil);
    QCOMPARE(this->lua.get<T*>(-1), nullptr);
    QVERIFY(!U::check_type(this->lua, -1));
    QVERIFY(UP::check_type(this->lua, -1));
    this->lua.pop();
    ::user_type u = {};
    this->lua.push(&u);
    QCOMPARE(this->lua.top(), 1);
    QCOMPARE(this->lua.get_type(-1), nngn::lua::type::user_data);
    QVERIFY(!lua_islightuserdata(this->lua, -1));
    QCOMPARE(this->lua.get<T*>(-1), &u);
    QVERIFY(U::check_type(this->lua, -1));
    QVERIFY(UP::check_type(this->lua, -1));
    this->lua.push(&u);
    QVERIFY(lua_compare(this->lua, -2, -1, LUA_OPEQ));
    this->lua.destroy();
}

void StackTest::user_type_ref(void) {
    using T = ::user_type;
    using U = nngn::lua::user_data<T>;
    using UP = nngn::lua::user_data<T*>;
    this->lua.new_user_type<T>();
    ::user_type u = {};
    QCOMPARE(nngn::lua::stack_push<T&>::push(this->lua, u), 1);
    QCOMPARE(this->lua.top(), 1);
    QCOMPARE(this->lua.get_type(-1), nngn::lua::type::user_data);
    QVERIFY(!lua_islightuserdata(this->lua, -1));
    QCOMPARE(&this->lua.get<T&>(-1), &u);
    QVERIFY(U::check_type(this->lua, -1));
    QVERIFY(UP::check_type(this->lua, -1));
    this->lua.destroy();
}

void StackTest::user_type_light(void) {
    using T = ::user_type;
    this->lua.new_user_type<T>();
    T u = {};
    const auto *ret = nngn::lua::user_data<T>::from_light(
        this->lua.push(&u).get<void*>());
    QCOMPARE(ret, &u);
}

void StackTest::user_type_align(void) {
    using T = ::user_type_align;
    this->lua.new_user_type<T>();
    this->lua.push(T{});
    QVERIFY(nngn::Math::is_aligned(this->lua.get<T*>(-1), alignof(T)));
}

void StackTest::user_type_destructor(void) {
    bool destroyed = false;
    const auto f = [this, &d = destroyed](auto &&x) {
        d = false;
        this->lua.push<nngn::lua::value>(FWD(x));
        lua_gc(this->lua, LUA_GCCOLLECT);
    };
    using T = ::user_type_destructor;
    this->lua.new_user_type<T>();
    f(T{&destroyed});
    QVERIFY(destroyed);
    auto u = T{&destroyed};
    f(&u);
    QVERIFY(!destroyed);
    this->lua.destroy();
}

void StackTest::tuple(void) {
    std::tuple t = std::tuple{true, 42, 43, std::string_view{"test"}};
    this->lua.push(t);
    QCOMPARE(lua_gettop(this->lua), 4);
    QCOMPARE(lua_type(this->lua, 1), LUA_TBOOLEAN);
    QCOMPARE(lua_type(this->lua, 2), LUA_TNUMBER);
    QCOMPARE(lua_type(this->lua, 3), LUA_TNUMBER);
    QCOMPARE(lua_type(this->lua, 4), LUA_TSTRING);
    QCOMPARE(this->lua.get<decltype(t)>(1), t);
}

void StackTest::error(void) {
    bool destroyed = false;
    this->lua.push([](void *p) {
        NNGN_SCOPED([p] { *static_cast<bool*>(p) = true; });
        return nngn::lua::error{"error"};
    });
    this->lua.push(static_cast<void*>(&destroyed));
    const int ret = lua_pcall(this->lua, 1, 0, 0);
    QCOMPARE(ret, LUA_ERRRUN);
    QCOMPARE(this->lua.top(), 1);
    QCOMPARE((lua_tostring(this->lua, -1)), "error");
    QVERIFY(destroyed);
}

void StackTest::variant(void) {
    using E = nngn::lua::error<const char*>;
    using V = std::variant<lua_Integer, E>;
    bool destroyed = false;
    const auto f = [this, &destroyed](bool arg) {
        destroyed = false;
        this->lua.push([](void *p, bool b) {
            NNGN_SCOPED([p] { *static_cast<bool*>(p) = true; });
            return b ? V{42} : V{E{"error"}};
        });
        this->lua.push(static_cast<void*>(&destroyed));
        this->lua.push(arg);
        return lua_pcall(this->lua, 2, 1, 0);
    };
    QCOMPARE(f(true), LUA_OK);
    QVERIFY(destroyed);
    QCOMPARE(this->lua.top(), 1);
    QCOMPARE(this->lua.get<nngn::lua::value>(-1), 42);
    QCOMPARE(f(false), LUA_ERRRUN);
    QVERIFY(destroyed);
    QCOMPARE(this->lua.top(), 1);
    QCOMPARE(this->lua.get(-1), "error"sv);
}

void StackTest::vector(void) {
    using T = std::vector<lua_Integer>;
    const auto t = nngn::lua::table_array(this->lua, 42, 43, 44, 45);
    const auto ret = this->lua.get<T>(t.index());
    const T expected = {42, 43, 44, 45};
    QCOMPARE(ret, expected);
}

void StackTest::type(void) {
    QCOMPARE(this->lua.push(42).get_type(), nngn::lua::type::number);
}

void StackTest::remove(void) {
    nngn::lua::value v = this->lua.push(nngn::lua::nil);
    QCOMPARE(this->lua.top(), 1);
    v.remove();
    QCOMPARE(this->lua.top(), 0);
}

void StackTest::str(void) {
    QVERIFY(this->lua.dostring("function f() end"));
    const auto f = [this] { this->lua.print_stack(); };
    QCOMPARE(nngn::Log::capture(f), "print_stack: top: 0\n");
    lua_pushnil(this->lua);
    this->lua.push(true);
    this->lua.push(static_cast<void*>(this));
    this->lua.push(42);
    this->lua.push(43.0);
    this->lua.push("str");
    this->lua.create_table().release();
    lua_getglobal(this->lua, "f");
    this->lua.push(+[](lua_State*) { return 0; });
    lua_newuserdatauv(this->lua, 0, 0);
    lua_pushthread(this->lua);
    QCOMPARE(this->lua.top(), 11);
    const auto ret = nngn::regexp_replace(
        nngn::Log::capture(f), std::regex{"0x[0-9a-f]+"}, "0x0");
    constexpr auto expected =
        "print_stack: top: 11\n"
        "  11 thread (thread: 0x0)\n"
        "  10 user_data (userdata: 0x0)\n"
        "  9 function (cfunction: 0x0)\n"
        "  8 function (function: 0x0)\n"
        "  7 table (table: 0x0)\n"
        "  6 string (str)\n"
        "  5 number (43.0)\n"
        "  4 number (42)\n"
        "  3 light_user_data (userdata: 0x0)\n"
        "  2 boolean (true)\n"
        "  1 nil (nil)\n";
    QCOMPARE(ret, expected);
}

QTEST_MAIN(StackTest)
