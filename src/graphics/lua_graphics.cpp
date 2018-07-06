#include "lua/state.h"

#include "graphics.h"

using nngn::Graphics;

NNGN_LUA_PROXY(Graphics,
    "PSEUDOGRAPH", nngn::lua::var(Graphics::Backend::PSEUDOGRAPH),
    "GLFW_BACKEND", nngn::lua::var(Graphics::Backend::GLFW_BACKEND),
    "swap_interval", &Graphics::swap_interval,
    "set_swap_interval", &Graphics::set_swap_interval)
