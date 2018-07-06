#include "lua/function.h"
#include "lua/register.h"
#include "lua/table.h"

#include "graphics.h"

using nngn::Graphics;

namespace {

void register_graphics(nngn::lua::table_view t) {
    t["PSEUDOGRAPH"] = Graphics::Backend::PSEUDOGRAPH;
    t["GLFW_BACKEND"] = Graphics::Backend::GLFW_BACKEND;
    t["swap_interval"] = &Graphics::swap_interval;
    t["set_swap_interval"] = &Graphics::set_swap_interval;
}

}

NNGN_LUA_DECLARE_USER_TYPE(Graphics)
NNGN_LUA_PROXY(Graphics, register_graphics)
