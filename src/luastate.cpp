#include <algorithm>
#include <cassert>
#include <sstream>

#include <lua.hpp>

#include "utils/log.h"

#include "luastate.h"

void LuaStack::pop(lua_State *L, int n) {
    assert(n <= lua_gettop(L));
    lua_pop(L, n);
}

std::vector<LuaState::register_f> &LuaState::get_registry()
    { static std::vector<register_f> r; return r; }

std::ostream &operator<<(std::ostream &os, const LuaState::Traceback &t) {
    luaL_traceback(t.L, t.L, nullptr, 0);
    os << lua_tostring(t.L, -1);
    lua_pop(t.L, 1);
    return os;
}

std::string LuaState::Traceback::str() const {
    std::stringstream s;
    s << *this;
    return s.str();
}

int LuaState::msgh(lua_State *L) {
    nngn::Log::l()
        << lua_tostring(L, -1) << ", "
        << LuaState::Traceback(L) << '\n';
    return 0;
}

void LuaState::print_stack(lua_State *L) {
    NNGN_LOG_CONTEXT_CF(LuaState);
    auto &l = nngn::Log::l();
    const auto n = lua_gettop(L);
    l << "top: " << n << '\n';
    for(auto i = n; i; --i) {
        l << "  " << i << ' ' << luaL_typename(L, i);
        switch(lua_type(L, i)) {
        case LUA_TNIL:
        case LUA_TSTRING:
        case LUA_TTABLE:
        case LUA_TFUNCTION:
        case LUA_TTHREAD: break;
        case LUA_TLIGHTUSERDATA:
        case LUA_TUSERDATA: l << ' ' << lua_touserdata(L, i); break;
        case LUA_TBOOLEAN: l << lua_toboolean(L, i); break;
        case LUA_TNUMBER:
            if(lua_isinteger(L, i))
                l << ' ' << lua_tointeger(L, i);
            else
                l << ' ' << lua_tonumber(L, i);
            break;
        }
        l << '\n';
    }
}

void LuaState::print_traceback(lua_State *L) {
    nngn::Log::l() << Traceback(L);
}

bool LuaState::init() {
    NNGN_LOG_CONTEXT_CF(LuaState);
    if(!(this->L = luaL_newstate()))
        return nngn::Log::l() << "failed\n", false;
    luaL_openlibs(this->L);
    this->register_all();
    return true;
}

void LuaState::destroy() {
    if(auto *l = std::exchange(this->L, nullptr))
        lua_close(l);
}

void LuaState::register_all() const {
    auto &v = LuaState::get_registry();
    for(auto x : v)
        x(this->L);
    v.clear();
}

void LuaState::log_traceback() const {
    LuaState::print_traceback(this->L);
}

static bool pcall(lua_State *L) {
    if(lua_pcall(L, 0, 0, 1) == LUA_OK) {
        lua_pop(L, 1);
        return true;
    }
    lua_pop(L, 2);
    return false;
}

bool LuaState::dofile(std::string_view f) const {
    NNGN_LOG_CONTEXT_CF(LuaState);
    NNGN_LOG_CONTEXT(f.data());
    lua_pushcfunction(this->L, LuaState::msgh);
    if(luaL_loadfile(this->L, f.data()) != LUA_OK) {
        nngn::Log::l() << lua_tostring(this->L, -1) << '\n';
        lua_pop(L, 2);
        return false;
    }
    return pcall(this->L);
}

bool LuaState::dostring(std::string_view src) const {
    NNGN_LOG_CONTEXT_CF(LuaState);
    NNGN_LOG_CONTEXT(src.data());
    lua_pushcfunction(this->L, LuaState::msgh);
    if(luaL_loadstring(this->L, src.data()) != LUA_OK) {
        nngn::Log::l() << lua_tostring(this->L, -1) << '\n';
        lua_pop(this->L, 2);
        return false;
    }
    return pcall(this->L);
}

bool LuaState::dofunction(std::string_view name) const {
    NNGN_LOG_CONTEXT_CF(LuaState);
    NNGN_LOG_CONTEXT(name.data());
    lua_pushcfunction(this->L, LuaState::msgh);
    if(lua_getglobal(this->L, name.data()) != LUA_TFUNCTION) {
        lua_pop(this->L, 2);
        return true;
    }
    return pcall(this->L);
}

bool LuaState::heartbeat() const {
    NNGN_LOG_CONTEXT_CF(LuaState);
    return this->dofunction("heartbeat");
}
