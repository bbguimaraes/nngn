#include <sol/state_view.hpp>
#include <sol/usertype_proxy.hpp>

#include "entity.h"
#include "luastate.h"
#include "player.h"

NNGN_LUA_PROXY(Player,
    sol::no_constructor,
    "FLEFT", sol::var(Player::FLEFT),
    "FRIGHT", sol::var(Player::FRIGHT),
    "FDOWN", sol::var(Player::FDOWN),
    "FUP", sol::var(Player::FUP),
    "N_FACES", sol::var(Player::N_FACES),
    "entity", [](const Player &p) { return p.e; },
    "face", [](const Player &p) { return p.face; },
    "set_entity", [](Player &p, Entity *e) { p.e = e; },
    "set_face", [](Player &p, Player::Face f) { p.face = f; },
    "face_vec", &Player::face_vec)
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
