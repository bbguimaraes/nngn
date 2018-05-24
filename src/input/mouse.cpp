#include "mouse.h"

#include <lua.hpp>

#include "lua/state.h"
#include "lua/utils.h"
#include "utils/log.h"

namespace {

void register_cb(lua_State *L, int *p) {
    if(*p)
        luaL_unref(L, LUA_REGISTRYINDEX, *p);
    lua_pushvalue(L, -1);
    *p = luaL_ref(L, LUA_REGISTRYINDEX);
}

}

namespace nngn {

void MouseInput::register_button_callback()
    { register_cb(this->L, &this->button_cb); }
void MouseInput::register_move_callback()
    { register_cb(this->L, &this->move_cb); }

bool MouseInput::button_callback(int button, int action, int mods) {
    if(!this->button_cb)
        return true;
    NNGN_LOG_CONTEXT_CF(MouseInput);
    const bool press = action == static_cast<int>(Action::PRESS);
    if(!press && action != static_cast<int>(Action::RELEASE))
        return true;
    lua_pushcfunction(L, nngn::lua::msgh);
    lua_rawgeti(L, LUA_REGISTRYINDEX, this->button_cb);
    lua_pushinteger(L, button);
    lua_pushboolean(L, press);
    lua_pushinteger(L, mods);
    NNGN_ANON_DECL(nngn::lua::defer_pop(L, 2));
    return lua_pcall(L, 3, 1, -5) == LUA_OK;
}

bool MouseInput::move_callback(dvec2 pos) {
    if(!this->move_cb)
        return true;
    NNGN_LOG_CONTEXT_CF(MouseInput);
    lua_pushcfunction(L, nngn::lua::msgh);
    lua_rawgeti(L, LUA_REGISTRYINDEX, this->move_cb);
    lua_pushnumber(L, pos.x);
    lua_pushnumber(L, pos.y);
    NNGN_ANON_DECL(nngn::lua::defer_pop(L, 2));
    return lua_pcall(L, 2, 1, -4) == LUA_OK;
}

}
