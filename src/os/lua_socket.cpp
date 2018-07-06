#include <sol/state_view.hpp>
#include <sol/usertype_proxy.hpp>

#include "luastate.h"

#include "socket.h"

using nngn::Socket;

NNGN_LUA_PROXY(Socket,
    sol::no_constructor,
    "init", &Socket::init)
