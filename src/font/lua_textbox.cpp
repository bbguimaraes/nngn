#include <sol/state_view.hpp>
#include <sol/usertype_proxy.hpp>

#include "luastate.h"

#include "textbox.h"

using nngn::Textbox;

namespace {

#define C(v) v ## _STR = {static_cast<char>(Textbox::Command::v), 0}
constexpr std::array<char, 2>
    C(TEXT_WHITE), C(TEXT_RED), C(TEXT_GREEN), C(TEXT_BLUE);
#undef C

}

NNGN_LUA_PROXY(Textbox,
    sol::no_constructor,
    "TEXT_WHITE", sol::var(Textbox::Command::TEXT_WHITE),
    "TEXT_RED", sol::var(Textbox::Command::TEXT_RED),
    "TEXT_GREEN", sol::var(Textbox::Command::TEXT_GREEN),
    "TEXT_BLUE", sol::var(Textbox::Command::TEXT_BLUE),
    "TEXT_WHITE_STR", sol::var(TEXT_WHITE_STR.data()),
    "TEXT_RED_STR", sol::var(TEXT_RED_STR.data()),
    "TEXT_GREEN_STR", sol::var(TEXT_GREEN_STR.data()),
    "TEXT_BLUE_STR", sol::var(TEXT_BLUE_STR.data()),
    "DEFAULT_SPEED", sol::var(Textbox::DEFAULT_SPEED.count()),
    "title", [](const Textbox &t) { return t.title.str; },
    "text", [](const Textbox &t) { return t.str.str; },
    "empty", &Textbox::empty,
    "finished", &Textbox::finished,
    "speed", [](const Textbox &t) { return t.speed.count(); },
    "set_speed", &Textbox::set_speed,
    "set_text", &Textbox::set_text,
    "set_title", &Textbox::set_title,
    "set_cur", &Textbox::set_cur)
