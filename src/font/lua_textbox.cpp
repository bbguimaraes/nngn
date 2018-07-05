#include <sol/state_view.hpp>
#include <sol/usertype_proxy.hpp>

#include "luastate.h"

#include "textbox.h"

using nngn::Textbox;

NNGN_LUA_PROXY(Textbox,
    sol::no_constructor,
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
