#include "lua/state.h"

#include "group.h"
#include "input.h"

using nngn::Input;

namespace {

using Action = Input::Action;
using Modifier = Input::Modifier;
using Selector = Input::Selector;

auto get_keys(Input &i, nngn::lua::as_table_t<std::vector<int32_t>> t) {
    auto &v = t.value();
    const auto n = v.size();
    i.get_keys(n, v.data());
    return t;
}

}

NNGN_LUA_PROXY(Input,
    "SEL_PRESS", nngn::lua::var(Input::Selector::PRESS),
    "SEL_CTRL", nngn::lua::var(Input::Selector::CTRL),
    "SEL_ALT", nngn::lua::var(Input::Selector::ALT),
    "MOD_SHIFT", nngn::lua::var(Input::MOD_SHIFT),
    "MOD_CTRL", nngn::lua::var(Input::MOD_CTRL),
    "MOD_ALT", nngn::lua::var(Input::MOD_ALT),
    "KEY_PRESS", nngn::lua::var(Input::KEY_PRESS),
    "KEY_RELEASE", nngn::lua::var(Input::KEY_RELEASE),
    "KEY_ESC", nngn::lua::var(Input::KEY_ESC),
    "KEY_ENTER", nngn::lua::var(Input::KEY_ENTER),
    "KEY_TAB", nngn::lua::var(Input::KEY_TAB),
    "KEY_RIGHT", nngn::lua::var(Input::KEY_RIGHT),
    "KEY_LEFT", nngn::lua::var(Input::KEY_LEFT),
    "KEY_DOWN", nngn::lua::var(Input::KEY_DOWN),
    "KEY_UP", nngn::lua::var(Input::KEY_UP),
    "KEY_PAGE_UP", nngn::lua::var(Input::KEY_PAGE_UP),
    "KEY_PAGE_DOWN", nngn::lua::var(Input::KEY_PAGE_DOWN),
    "OUTPUT_PROCESSING", nngn::lua::var(Input::TerminalFlag::OUTPUT_PROCESSING),
    "terminal_source", [](int fd, std::optional<Input::TerminalFlag> flags) {
        return nngn::input_terminal_source(
            fd, flags.value_or(Input::TerminalFlag{}));
    },
    "graphics_source", &nngn::input_graphics_source,
    "binding_group", &Input::binding_group,
    "get_keys", get_keys,
    "set_binding_group", &Input::set_binding_group,
    "add_source", [](Input &i, std::unique_ptr<Input::Source> &s) {
        i.add_source(std::move(s));
    },
    "remove_source", &Input::remove_source,
    "register_callback", &Input::register_callback,
    "remove_callback", &Input::remove_callback,
    "key_callback", &Input::key_callback)
