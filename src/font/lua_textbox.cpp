#include "lua/function.h"
#include "lua/register.h"
#include "lua/table.h"

#include "textbox.h"

using nngn::Textbox;

namespace {

#define C(v) v ## _STR = {static_cast<char>(Textbox::Command::v), 0}
constexpr std::array<char, 2>
    C(TEXT_WHITE), C(TEXT_RED), C(TEXT_GREEN), C(TEXT_BLUE);
#undef C

void set_cur(Textbox &t, lua_Integer i) {
    t.set_cur(nngn::narrow<std::size_t>(i));
}

void register_textbox(nngn::lua::table_view t) {
    t["TEXT_WHITE"] = Textbox::Command::TEXT_WHITE;
    t["TEXT_RED"] = Textbox::Command::TEXT_RED;
    t["TEXT_GREEN"] = Textbox::Command::TEXT_GREEN;
    t["TEXT_BLUE"] = Textbox::Command::TEXT_BLUE;
    t["TEXT_WHITE_STR"] = TEXT_WHITE_STR.data();
    t["TEXT_RED_STR"] = TEXT_RED_STR.data();
    t["TEXT_GREEN_STR"] = TEXT_GREEN_STR.data();
    t["TEXT_BLUE_STR"] = TEXT_BLUE_STR.data();
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
