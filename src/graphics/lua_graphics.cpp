#include "lua/state.h"

#include "graphics.h"

using nngn::Graphics;

NNGN_LUA_PROXY(Graphics,
    "PSEUDOGRAPH", nngn::lua::var(Graphics::Backend::PSEUDOGRAPH),
    "GLFW_BACKEND", nngn::lua::var(Graphics::Backend::GLFW_BACKEND),
    "CURSOR_MODE_NORMAL", nngn::lua::var(Graphics::CursorMode::NORMAL),
    "CURSOR_MODE_HIDDEN", nngn::lua::var(Graphics::CursorMode::HIDDEN),
    "CURSOR_MODE_DISABLED", nngn::lua::var(Graphics::CursorMode::DISABLED),
    "swap_interval", &Graphics::swap_interval,
    "set_swap_interval", &Graphics::set_swap_interval,
    "set_cursor_mode", &Graphics::set_cursor_mode)
