/**
 * \dir src
 * \brief Main source directory (C++20, Lua).
 *
 * Only a minimal amount of core source lives directly under this directory.
 * Most code is placed under modules in each subdirectory (see the documentation
 * of each of them for details).  Dependencies between modules are kept to a
 * minimum and with few exceptional cases the dependency graph is a DAG.
 *
 * ## Configuration
 *
 * All configuration is done using command-line arguments, and all command-line
 * argument processing is done using Lua.  Each argument is processed similarly
 * to how the standalone Lua interpreter processes the `LUA_INIT*` variables:
 *
 * - Each argument is executed in sequence.
 * - An argument with the format `@filename` causes `filename` to be read and
 *   executed as Lua code.
 * - Any other argument is executed literally as Lua code.
 * - Failure to execute any of the arguments aborts the initialization process.
 *   A failure exit code is returned.
 */
#include <algorithm>

#include "graphics/graphics.h"
#include "lua/function.h"
#include "lua/register.h"
#include "lua/table.h"
#include "math/math.h"
#include "os/platform.h"
#include "os/socket.h"
#include "timing/fps.h"
#include "timing/timing.h"
#include "utils/flags.h"
#include "utils/log.h"

NNGN_LUA_DECLARE_USER_TYPE(nngn::Math, "Math")
NNGN_LUA_DECLARE_USER_TYPE(nngn::Timing, "Timing")
NNGN_LUA_DECLARE_USER_TYPE(nngn::Graphics, "Graphics")
NNGN_LUA_DECLARE_USER_TYPE(nngn::FPS, "FPS")
NNGN_LUA_DECLARE_USER_TYPE(nngn::Socket, "Socket")
NNGN_LUA_DECLARE_USER_TYPE(nngn::lua::state, "state")

namespace {

struct NNGN {
    enum Flag : uint8_t {
        EXIT = 1u << 0, ERROR = 1u << 1,
    };
    nngn::Flags<Flag> flags = {};
    nngn::Math math = {};
    nngn::Timing timing = {};
    std::unique_ptr<nngn::Graphics> graphics = {};
    nngn::FPS fps = {};
    nngn::Socket socket = {};
    nngn::lua::state lua = {};
    bool init(int argc, const char *const *argv);
    bool set_graphics(nngn::Graphics::Backend b, const void *params);
    int loop(void);
    void exit(void) { this->flags |= Flag::EXIT; }
    void die(void) { this->flags |= Flag::ERROR; }
} *p_nngn;

}

NNGN_LUA_DECLARE_USER_TYPE(NNGN)

namespace {

bool NNGN::init(int argc, const char *const *argv) {
    NNGN_LOG_CONTEXT_CF(NNGN);
    this->math.init();
    if(!nngn::Platform::init(argc, argv))
        return false;
    if(!this->lua.init())
        return false;
    nngn::lua::static_register::register_all(this->lua);
    const auto L = nngn::lua::global_table{this->lua};
    L["nngn"] = this;
    L["deref"] = [](const void *pp) {
        const void *p = *static_cast<const void *const*>(pp);
        return static_cast<lua_Integer>(reinterpret_cast<std::uintptr_t>(p));
    };
    L["log"] = [](lua_State *L_) {
        auto &l = nngn::Log::l();
        for(int i = 1, t = lua_gettop(L_); i <= t; ++i)
            l << lua_tostring(L_, i);
        return 0;
    };
    if(!(argc < 2
        ? this->lua.dofile("src/lua/all.lua")
        : std::all_of(
            std::next(begin(nngn::Platform::argv)),
            end(nngn::Platform::argv),
            NNGN_BIND_MEM_FN(&this->lua, doarg))
    ))
        return false;
    this->fps.init(nngn::Timing::clock::now());
    if(!this->graphics)
        this->set_graphics(nngn::Graphics::Backend::PSEUDOGRAPH, {});
    return true;
}

bool NNGN::set_graphics(nngn::Graphics::Backend b, const void *params) {
    auto g = nngn::Graphics::create(
        b, nngn::lua::user_data<char>::from_light(params));
    if(!g || !g->init())
        return false;
    this->graphics = std::move(g);
    return true;
}

int NNGN::loop(void) {
    if(this->flags.is_set(Flag::ERROR))
        return 1;
    if(this->flags.is_set(Flag::EXIT) || this->graphics->window_closed())
        return 0;
    this->timing.update();
    this->graphics->poll_events();
    bool ok = true;
    ok = ok && this->socket.process(
        [&l = this->lua](auto s) { l.dostring(s); });
    ok = ok && this->graphics->render();
    if(!ok)
        return 1;
    this->fps.frame(nngn::Timing::clock::now());
    this->graphics->set_window_title(this->fps.to_string().c_str());
    return -1;
}

void register_nngn(nngn::lua::table &&t) {
    using nngn::lua::accessor;
    t["math"] = accessor<&NNGN::math>;
    t["timing"] = accessor<&NNGN::timing>;
    t["graphics"] = [](NNGN &nngn) { return nngn.graphics.get(); };
    t["fps"] = accessor<&NNGN::fps>;
    t["socket"] = accessor<&NNGN::socket>;
    t["lua"] = accessor<&NNGN::lua>;
    t["set_graphics"] = &NNGN::set_graphics;
    t["exit"] = &NNGN::exit;
    t["die"] = &NNGN::die;
}

NNGN_LUA_PROXY(NNGN, register_nngn)

}

#ifdef NNGN_PLATFORM_EMSCRIPTEN
extern "C" {
void lua(const char *s) { NNGN_LOG_CONTEXT_F(); p_nngn->lua.dostring(s); }
}
#endif

int main(int argc, const char *const *argv) {
    NNGN nngn;
    p_nngn = &nngn;
    if(!nngn.init(argc, argv))
        return 1;
    return nngn::Platform::loop([]() { return p_nngn->loop(); });
}
