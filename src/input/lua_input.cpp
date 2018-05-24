#include "lua/function.h"
#include "lua/register.h"
#include "lua/table.h"

#include "graphics/graphics.h"

#include "group.h"
#include "input.h"

using nngn::i32;
using nngn::Input;

NNGN_LUA_DECLARE_USER_TYPE(nngn::Graphics, "Graphics")
NNGN_LUA_DECLARE_USER_TYPE(nngn::BindingGroup, "BindingGroup")
NNGN_LUA_DECLARE_USER_TYPE(Input::Source)

namespace {

using Action = Input::Action;
using Modifier = Input::Modifier;
using Selector = Input::Selector;

nngn::lua::table_view get_keys(
    Input &i, std::vector<i32> v, nngn::lua::state_view lua)
{
    i.get_keys(v);
    return nngn::lua::table_from_range(lua, v).release();
}

auto terminal_source(int fd, std::optional<Input::TerminalFlag> flags)  {
    auto f = flags.value_or(Input::TerminalFlag{});
    return nngn::input_terminal_source(fd, f).release();
}

auto input_graphics_source(nngn::Graphics *g) {
    return nngn::input_graphics_source(g).release();
}

void add_source(Input &i, Input::Source *s) {
    i.add_source(std::unique_ptr<Input::Source>{s});
}

bool override_key(Input &i, bool pressed, lua_Integer k) {
    const auto ki = nngn::narrow<i32>(k);
    return i.override_keys(pressed, {&ki, 1});
}

bool override_keys(Input &i, bool pressed, std::vector<lua_Integer> k) {
    std::vector<i32> ki(k.size());
    std::transform(begin(k), end(k), begin(ki), nngn::narrow<lua_Integer, i32>);
    return i.override_keys(pressed, ki);
}

void register_input(nngn::lua::table_view t) {
    t["SEL_PRESS"] = Input::Selector::PRESS;
    t["SEL_CTRL"] = Input::Selector::CTRL;
    t["SEL_ALT"] = Input::Selector::ALT;
    t["MOD_SHIFT"] = Input::MOD_SHIFT;
    t["MOD_CTRL"] = Input::MOD_CTRL;
    t["MOD_ALT"] = Input::MOD_ALT;
    t["KEY_PRESS"] = Input::KEY_PRESS;
    t["KEY_RELEASE"] = Input::KEY_RELEASE;
    t["KEY_ESC"] = Input::KEY_ESC;
    t["KEY_ENTER"] = Input::KEY_ENTER;
    t["KEY_TAB"] = Input::KEY_TAB;
    t["KEY_RIGHT"] = Input::KEY_RIGHT;
    t["KEY_LEFT"] = Input::KEY_LEFT;
    t["KEY_DOWN"] = Input::KEY_DOWN;
    t["KEY_UP"] = Input::KEY_UP;
    t["KEY_PAGE_UP"] = Input::KEY_PAGE_UP;
    t["KEY_PAGE_DOWN"] = Input::KEY_PAGE_DOWN;
    t["OUTPUT_PROCESSING"] = Input::TerminalFlag::OUTPUT_PROCESSING;
    t["terminal_source"] = terminal_source;
    t["graphics_source"] = input_graphics_source;
    t["binding_group"] = &Input::binding_group;
    t["get_keys"] = get_keys;
    t["set_binding_group"] = &Input::set_binding_group;
    t["add_source"] = add_source;
    t["remove_source"] = &Input::remove_source;
    t["override_key"] = override_key;
    t["override_keys"] = override_keys;
    t["register_callback"] = &Input::register_callback;
    t["remove_callback"] = &Input::remove_callback;
    t["key_callback"] = &Input::key_callback;
}

}

NNGN_LUA_DECLARE_USER_TYPE(Input)
NNGN_LUA_PROXY(Input::Source)
NNGN_LUA_PROXY(Input, register_input)
