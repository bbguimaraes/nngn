#include <sol/state_view.hpp>
#include <sol/usertype_proxy.hpp>

#include "luastate.h"

#include "graphics.h"

using nngn::Graphics;

NNGN_LUA_PROXY(Graphics,
    sol::no_constructor,
    "PSEUDOGRAPH", sol::var(Graphics::Backend::PSEUDOGRAPH),
    "GLFW_BACKEND", sol::var(Graphics::Backend::GLFW_BACKEND),
    "swap_interval", &Graphics::swap_interval,
    "set_swap_interval", &Graphics::set_swap_interval)
