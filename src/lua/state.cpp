#include "state.h"

#include "utils/log.h"

#include "traceback.h"
#include "utils.h"

namespace {

bool pcall(lua_State *L, auto &&f) {
    NNGN_ANON_DECL(nngn::lua::stack_mark(L));
    lua_pushcfunction(L, nngn::lua::msgh);
    auto pop = nngn::lua::defer_pop{L};
    if(!FWD(f)())
        return false;
    if(lua_pcall(L, 0, 0, -2) == LUA_OK)
        return true;
    pop.set_n(2);
    return false;
}

}

namespace nngn::lua {

int msgh(lua_State *L) {
    NNGN_LOG_CONTEXT_F();
    const char *s = lua_tostring(L, -1);
    if(!s) {
        luaL_tolstring(L, -1, nullptr);
        s = lua_tostring(L, -1);
    }
    Log::l() << s << ", " << traceback(L) << '\n';
    return 0;
}

void print_stack(lua_State *L) {
    NNGN_LOG_CONTEXT_F();
    auto &l = Log::l();
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

void print_traceback(lua_State *L) {
    Log::l() << traceback(L);
}

void static_register::register_all(lua_State *L) {
    auto &v = static_register::registry();
    for(auto x : v)
        x(L);
    v.clear();
}

bool state_view::init(void) {
    NNGN_LOG_CONTEXT_CF(lua::state_view);
    this->destroy();
    if(!(this->L = luaL_newstate()))
        return Log::l() << "failed\n", false;
    luaL_openlibs(this->L);
    return true;
}

void state_view::destroy(void) {
    NNGN_LOG_CONTEXT_CF(lua::state_view);
    if(auto *const l = std::exchange(this->L, nullptr))
        lua_close(l);
}

bool state_view::doarg(std::string_view s) const {
    return s[0] == '@' ? this->dofile(s.substr(1)) : this->dostring(s);
}

bool state_view::dofile(std::string_view f) const {
    NNGN_LOG_CONTEXT_CF(lua::state_view);
    NNGN_LOG_CONTEXT(f.data());
    return ::pcall(this->L, [this, f] {
        if(luaL_loadfile(this->L, f.data()) == LUA_OK)
            return true;
        Log::l() << "failed to load file: " << lua_tostring(L, -1) << '\n';
        lua_pop(L, 1);
        return false;
    });
}

bool state_view::dostring(std::string_view src) const {
    NNGN_LOG_CONTEXT_CF(lua::state_view);
    NNGN_LOG_CONTEXT(src.data());
    return ::pcall(this->L, [this, src] {
        if(luaL_loadstring(this->L, src.data()) == LUA_OK)
            return true;
        Log::l() << "failed to load string: " << lua_tostring(L, -1) << '\n';
        lua_pop(L, 1);
        return false;
    });
}

bool state_view::dofunction(std::string_view f) const {
    NNGN_LOG_CONTEXT_CF(lua::state_view);
    NNGN_LOG_CONTEXT(f.data());
    return ::pcall(this->L, [this, f] {
        if(lua_getglobal(this->L, f.data()) == LUA_TFUNCTION)
            return true;
        Log::l() << "failed to find function: " << f.data() << '\n';
        return false;
    });
}

bool state_view::heartbeat() const {
    NNGN_LOG_CONTEXT_CF(lua::state_view);
    NNGN_ANON_DECL(stack_mark(this->L));
    auto pop = defer_pop{this->L};
    lua_pushcfunction(this->L, nngn::lua::msgh);
    if(lua_getglobal(this->L, "heartbeat") != LUA_TFUNCTION)
        return true;
    pop.set_n(2);
    if(lua_pcall(this->L, 0, 0, 1) == LUA_OK)
        return true;
    Log::l() << lua_tostring(this->L, -1) << '\n';
    return false;
}

}
