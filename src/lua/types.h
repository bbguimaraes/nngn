#ifndef NNGN_LUA_TYPES_H
#define NNGN_LUA_TYPES_H

#include <lua.hpp>

namespace nngn::lua {

enum class type {
    none = LUA_TNONE,
    nil = LUA_TNIL,
    boolean = LUA_TBOOLEAN,
    light_userdata = LUA_TLIGHTUSERDATA,
    number = LUA_TNUMBER,
    string = LUA_TSTRING,
    table = LUA_TTABLE,
    function = LUA_TFUNCTION,
    userdata = LUA_TUSERDATA,
    thread = LUA_TTHREAD,
};

}

#endif
