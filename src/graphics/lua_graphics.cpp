#include "lua/function.h"
#include "lua/iter.h"
#include "lua/register.h"
#include "lua/table.h"

#include "utils/log.h"

#include "graphics.h"

using nngn::Graphics;

NNGN_LUA_DECLARE_USER_TYPE(Graphics::OpenGLParameters, "OpenGLParameters")

namespace {

std::optional<Graphics::OpenGLParameters> opengl_params(
    nngn::lua::table_view t)
{
    NNGN_LOG_CONTEXT_F();
    Graphics::OpenGLParameters ret = {};
    for(const auto &[k, v] : t) {
        const auto ks = k.get<std::optional<std::string_view>>();
        if(!ks) {
            nngn::Log::l() << "only string keys are allowed\n";
            return {};
        }
        using Flag = Graphics::Parameters::Flag;
        if(*ks == "hidden")
            ret.flags.set(Flag::HIDDEN, v.get<bool>());
        else if(*ks == "debug")
            ret.flags.set(Flag::DEBUG, v.get<bool>());
        else if(*ks == "maj")
            ret.maj = v.get<int>();
        else if(*ks == "min")
            ret.min = v.get<int>();
    }
    return ret;
}

auto version(const Graphics &g) {
    const auto v = g.version();
    return std::tuple{v.major, v.minor, v.patch, v.name};
}

void register_graphics(nngn::lua::table_view t) {
    t["PSEUDOGRAPH"] = Graphics::Backend::PSEUDOGRAPH;
    t["OPENGL_BACKEND"] = Graphics::Backend::OPENGL_BACKEND;
    t["OPENGL_ES_BACKEND"] = Graphics::Backend::OPENGL_ES_BACKEND;
    t["CURSOR_MODE_NORMAL"] = Graphics::CursorMode::NORMAL;
    t["CURSOR_MODE_HIDDEN"] = Graphics::CursorMode::HIDDEN;
    t["CURSOR_MODE_DISABLED"] = Graphics::CursorMode::DISABLED;
    t["opengl_params"] = opengl_params;
    t["version"] = version;
    t["error"] = &Graphics::error;
    t["swap_interval"] = &Graphics::swap_interval;
    t["set_swap_interval"] = &Graphics::set_swap_interval;
    t["set_cursor_mode"] = &Graphics::set_cursor_mode;
}

}

NNGN_LUA_DECLARE_USER_TYPE(Graphics::Parameters)
NNGN_LUA_DECLARE_USER_TYPE(Graphics)
NNGN_LUA_PROXY(Graphics, register_graphics)
NNGN_LUA_PROXY(Graphics::Parameters)
NNGN_LUA_PROXY(Graphics::OpenGLParameters)
