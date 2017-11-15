#include <sol/state_view.hpp>
#include <sol/usertype_proxy.hpp>

#include "entity.h"
#include "luastate.h"
#include "player.h"

NNGN_LUA_PROXY(Player,
    sol::no_constructor,
    "FACE", sol::var(Player::Animation::FACE),
    "FLEFT", sol::var(Player::Face::FLEFT),
    "FRIGHT", sol::var(Player::Face::FRIGHT),
    "FDOWN", sol::var(Player::Face::FDOWN),
    "FUP", sol::var(Player::Face::FUP),
    "N_FACES", sol::var(Player::N_FACES),
    "WALK", sol::var(Player::Animation::WALK),
    "WLEFT", sol::var(Player::Animation::WLEFT),
    "WRIGHT", sol::var(Player::Animation::WRIGHT),
    "WDOWN", sol::var(Player::Animation::WDOWN),
    "WUP", sol::var(Player::Animation::WUP),
    "RUN", sol::var(Player::Animation::RUN),
    "RLEFT", sol::var(Player::Animation::RLEFT),
    "RRIGHT", sol::var(Player::Animation::RRIGHT),
    "RDOWN", sol::var(Player::Animation::RDOWN),
    "RUP", sol::var(Player::Animation::RUP),
    "N_ANIMATIONS", sol::var(Player::N_ANIMATIONS),
    "entity", [](const Player &p) { return p.e; },
    "face", [](const Player &p) { return p.face; },
    "running", [](const Player &p) { return p.running; },
    "set_entity", [](Player &p, Entity *e) { p.e = e; },
    "set_face", [](Player &p, Player::Face f) { p.face = f; },
    "set_running", [](Player &p, bool r) { p.running = r; },
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
