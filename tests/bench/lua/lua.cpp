#include "lua.h"

#include <lua.hpp>

#include "utils/utils.h"

namespace {

constexpr int N = NNGN_BENCH_LUA_N;

void repeat(auto &&f) { for(int i = 0; i != N; ++i) f(); }
void bench(auto &&f) { QBENCHMARK { repeat(FWD(f)); } }

template<auto f>
void registry_access(auto *L) {
    lua_pushboolean(L, true);
    const int r = luaL_ref(L, LUA_REGISTRYINDEX);
    QVERIFY(lua_checkstack(L, N));
    QBENCHMARK {
        repeat([L, r] {
            lua_geti(L, LUA_REGISTRYINDEX, r);
            lua_toboolean(L, 1);
        });
        lua_pop(L, N);
    }
    luaL_unref(L, LUA_REGISTRYINDEX, r);
}

}

void LuaBench::initTestCase(void) { this->L = luaL_newstate(); }
void LuaBench::cleanupTestCase(void) { lua_close(std::exchange(this->L, {})); }

void LuaBench::base(void) {
    lua_pushboolean(this->L, true);
    bench([this] {
        lua_pushvalue(this->L, -1);
        lua_pop(this->L, 1);
    });
    lua_pop(this->L, 1);
}

void LuaBench::access_stack(void) {
    lua_pushboolean(this->L, true);
    bench([this] { lua_toboolean(this->L, 1); });
    lua_pop(this->L, 1);
}

void LuaBench::access_registry(void) {
    ::registry_access<lua_geti>(this->L);
}

void LuaBench::access_registry_raw(void) {
    ::registry_access<lua_rawgeti>(this->L);
}

void LuaBench::access_global(void) {
    lua_pushboolean(this->L, true);
    lua_setglobal(this->L, "x");
    QVERIFY(lua_checkstack(L, N));
    QBENCHMARK {
        repeat([this] {
            lua_getglobal(this->L, "x");
            lua_toboolean(this->L, 1);
        });
        lua_pop(this->L, N);
    }
    lua_pushnil(this->L);
    lua_setglobal(this->L, "x");
}

void LuaBench::push_stack(void) {
    QVERIFY(lua_checkstack(this->L, N));
    QBENCHMARK {
        repeat([this] { lua_pushboolean(this->L, true); });
        lua_pop(this->L, N);
    }
}

void LuaBench::push_registry(void) {
    lua_pushboolean(this->L, true);
    bench([this] {
        lua_pushvalue(this->L, -1);
        luaL_ref(this->L, LUA_REGISTRYINDEX);
    });
    lua_pop(this->L, 1);
}

void LuaBench::push_global(void) {
    lua_pushboolean(this->L, true);
    bench([this] {
        lua_pushvalue(this->L, -1);
        lua_setglobal(this->L, "x");
    });
    lua_pop(this->L, 1);
    lua_pushnil(this->L);
    lua_setglobal(L, "x");
}

QTEST_MAIN(LuaBench)
