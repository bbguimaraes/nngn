#include <sol/state_view.hpp>
#include <sol/usertype_proxy.hpp>

#include "luastate.h"

#include "graphics.h"

using nngn::Graphics;

NNGN_LUA_PROXY(Graphics,
    sol::no_constructor,
    "PSEUDOGRAPH", sol::var(Graphics::Backend::PSEUDOGRAPH),
    "GLFW_BACKEND", sol::var(Graphics::Backend::GLFW_BACKEND),
    "CURSOR_MODE_NORMAL", sol::var(Graphics::CursorMode::NORMAL),
    "CURSOR_MODE_HIDDEN", sol::var(Graphics::CursorMode::HIDDEN),
    "CURSOR_MODE_DISABLED", sol::var(Graphics::CursorMode::DISABLED),
    "swap_interval", &Graphics::swap_interval,
    "set_swap_interval", &Graphics::set_swap_interval,
    "set_cursor_mode", &Graphics::set_cursor_mode)
