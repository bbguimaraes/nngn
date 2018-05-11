#include <sol/state_view.hpp>
#include <sol/usertype_proxy.hpp>

#include "entity.h"
#include "luastate.h"
#include "player.h"

NNGN_LUA_PROXY(Player,
    sol::no_constructor,
    "entity", [](const Player &p) { return p.e; },
    "set_entity", [](Player &p, Entity *e) { p.e = e; })
NNGN_LUA_PROXY(Players,
    sol::no_constructor,
    "n", &Players::n,
    "idx", &Players::idx,
    "cur", &Players::cur,
    "set_idx", &Players::set_idx,
    "get", &Players::get,
    "set_cur", &Players::set_cur,
    "add", &Players::add,
    "remove", &Players::remove)
