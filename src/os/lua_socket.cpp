#include "lua/state.h"

#include "socket.h"

using nngn::Socket;

NNGN_LUA_PROXY(Socket,
    "init", &Socket::init)
