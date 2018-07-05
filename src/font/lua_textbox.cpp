#include "lua/function.h"
#include "lua/register.h"
#include "lua/table.h"

#include "textbox.h"

using nngn::Textbox;

namespace {

void set_cur(Textbox &t, lua_Integer i) {
    t.set_cur(nngn::narrow<std::size_t>(i));
}

void register_textbox(nngn::lua::table_view t) {
    t["DEFAULT_SPEED"] = Textbox::DEFAULT_SPEED.count();
    t["title"] = [](const Textbox &tb) { return tb.title.str; };
    t["text"] = [](const Textbox &tb) { return tb.str.str; };
    t["empty"] = &Textbox::empty;
    t["finished"] = &Textbox::finished;
    t["speed"] = [](const Textbox &tb) { return tb.speed.count(); };
    t["set_monospaced"] = &Textbox::set_monospaced;
    t["set_speed"] = &Textbox::set_speed;
    t["set_text"] = &Textbox::set_text;
    t["set_title"] = &Textbox::set_title;
    t["set_cur"] = set_cur;
}

}

NNGN_LUA_DECLARE_USER_TYPE(Textbox)
NNGN_LUA_PROXY(Textbox, register_textbox)
