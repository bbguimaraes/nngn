#include "lua/function.h"
#include "lua/register.h"
#include "lua/table.h"

#include "socket.h"

using nngn::Socket;

namespace {

void register_socket(nngn::lua::table_view t) {
    t["init"] = &Socket::init;
}

}

NNGN_LUA_DECLARE_USER_TYPE(Socket)
NNGN_LUA_PROXY(Socket, register_socket)
