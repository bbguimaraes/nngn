#include "lua/state.h"

#include "textbox.h"

using nngn::Textbox;

NNGN_LUA_PROXY(Textbox,
    "DEFAULT_SPEED", nngn::lua::var(Textbox::DEFAULT_SPEED.count()),
    "title", [](const Textbox &t) { return t.title.str; },
    "text", [](const Textbox &t) { return t.str.str; },
    "empty", &Textbox::empty,
    "finished", &Textbox::finished,
    "speed", [](const Textbox &t) { return t.speed.count(); },
    "set_monospaced", &Textbox::set_monospaced,
    "set_speed", &Textbox::set_speed,
    "set_text", &Textbox::set_text,
    "set_title", &Textbox::set_title,
    "set_cur", &Textbox::set_cur)
