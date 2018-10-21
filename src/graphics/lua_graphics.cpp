#include <optional>

#include <sol/state_view.hpp>
#include <sol/usertype_proxy.hpp>

#include "luastate.h"

#include "utils/log.h"

#include "graphics.h"

using nngn::Graphics;

namespace {

std::optional<Graphics::OpenGLParameters> opengl_params(
        const sol::stack_table &t) {
    NNGN_LOG_CONTEXT_F();
    Graphics::OpenGLParameters ret = {};
    for(const auto &[k, v] : t) {
        const auto ko = k.as<std::optional<std::string_view>>();
        if(!ko) {
            nngn::Log::l() << "only string keys are allowed\n";
            return std::nullopt;
        }
        const auto ks = *ko;
        if(ks == "hidden")
            ret.flags.set(Graphics::Parameters::Flag::HIDDEN, v.as<bool>());
        else if(ks == "debug")
            ret.flags.set(Graphics::Parameters::Flag::DEBUG, v.as<bool>());
        else if(ks == "maj")
            ret.maj = v.as<int>();
        else if(ks == "min")
            ret.min = v.as<int>();
    }
    return {ret};
}

}

NNGN_LUA_PROXY(Graphics,
    sol::no_constructor,
    "PSEUDOGRAPH", sol::var(Graphics::Backend::PSEUDOGRAPH),
    "OPENGL_BACKEND", sol::var(Graphics::Backend::OPENGL_BACKEND),
    "OPENGL_ES_BACKEND", sol::var(Graphics::Backend::OPENGL_ES_BACKEND),
    "CURSOR_MODE_NORMAL", sol::var(Graphics::CursorMode::NORMAL),
    "CURSOR_MODE_HIDDEN", sol::var(Graphics::CursorMode::HIDDEN),
    "CURSOR_MODE_DISABLED", sol::var(Graphics::CursorMode::DISABLED),
    "opengl_params", opengl_params,
    "version", &Graphics::version,
    "error", &Graphics::error,
    "swap_interval", &Graphics::swap_interval,
    "set_swap_interval", &Graphics::set_swap_interval,
    "set_cursor_mode", &Graphics::set_cursor_mode)
