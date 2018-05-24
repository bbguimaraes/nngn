#include "lua/function.h"
#include "lua/register.h"
#include "lua/table.h"

#include "graphics.h"

using nngn::Graphics;

namespace {

void register_graphics(nngn::lua::table_view t) {
    t["PSEUDOGRAPH"] = Graphics::Backend::PSEUDOGRAPH;
    t["GLFW_BACKEND"] = Graphics::Backend::GLFW_BACKEND;
    t["CURSOR_MODE_NORMAL"] = Graphics::CursorMode::NORMAL;
    t["CURSOR_MODE_HIDDEN"] = Graphics::CursorMode::HIDDEN;
    t["CURSOR_MODE_DISABLED"] = Graphics::CursorMode::DISABLED;
    t["swap_interval"] = &Graphics::swap_interval;
    t["set_swap_interval"] = &Graphics::set_swap_interval;
    t["set_cursor_mode"] = &Graphics::set_cursor_mode;
}

}

NNGN_LUA_DECLARE_USER_TYPE(Graphics)
NNGN_LUA_PROXY(Graphics, register_graphics)
