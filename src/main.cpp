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
#include "graphics/graphics.h"
#include "lua/alloc.h"
#include "lua/state.h"
#include "os/platform.h"
#include "os/socket.h"
#include "timing/fps.h"
#include "timing/profile.h"
#include "timing/schedule.h"
#include "timing/timing.h"
#include "utils/flags.h"
#include "utils/log.h"
#include "utils/scoped.h"

namespace {

struct NNGN {
    enum Flag : uint8_t {
        EXIT = 1u << 0, ERROR = 1u << 1,
    };
    nngn::Flags<Flag> flags = {};
    nngn::Timing timing = {};
    nngn::Schedule schedule = {};
    std::unique_ptr<nngn::Graphics> graphics = {};
    nngn::FPS fps = {};
    nngn::Socket socket = {};
    nngn::lua::state lua = {};
    nngn::lua::alloc_info lua_alloc = {};
    bool init(int argc, const char *const *argv);
    bool set_graphics(
        nngn::Graphics::Backend b, std::optional<const void*> params);
    int loop(void);
    void exit(void) { this->flags |= Flag::EXIT; }
    void die(void) { this->flags |= Flag::ERROR; }
} *p_nngn;

bool NNGN::init(int argc, const char *const *argv) {
    NNGN_LOG_CONTEXT_CF(NNGN);
    if(!nngn::Platform::init(argc, argv))
        return false;
    nngn::Profile::init();
    if(!this->lua.init(&this->lua_alloc))
        return false;
    nngn::lua::static_register::register_all(this->lua);
    const auto L = nngn::lua::global_table{this->lua};
    L["nngn"] = this;
    L["deref"] = +[](void *p) { return *static_cast<uintptr_t*>(p); };
    L["log"] = +[](lua_State *L_) {
        auto &l = nngn::Log::l();
        for(int i = 1, t = lua_gettop(L_); i <= t; ++i)
            l << lua_tostring(L_, i);
        return 0;
    };
    this->schedule.init(&this->timing);
    if(!(argc < 2
        ? this->lua.dofile("src/lua/all.lua")
        : std::ranges::all_of(
            nngn::Platform::argv.subspan(1),
            NNGN_BIND_MEM_FN(&this->lua, doarg))
    ))
        return false;
    this->fps.init(nngn::Timing::clock::now());
    if(!this->graphics)
        this->set_graphics(nngn::Graphics::Backend::PSEUDOGRAPH, {});
    return true;
}

bool NNGN::set_graphics(
        nngn::Graphics::Backend b, std::optional<const void*> params) {
    auto g = nngn::Graphics::create(
        b, params ? static_cast<const uintptr_t*>(*params) + 1 : nullptr);
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
    ok = ok && this->schedule.update();
    ok = ok && this->socket.process(
        [&l = this->lua](auto s) { l.dostring(s); });
    ok = ok && this->graphics->render();
    if(!ok)
        return 1;
    nngn::Profile::swap();
    this->fps.frame(nngn::Timing::clock::now());
    this->graphics->set_window_title(this->fps.to_string().c_str());
    return -1;
}

using nngn::lua::property, nngn::lua::readonly;

NNGN_LUA_PROXY(NNGN,
    "timing", readonly(&NNGN::timing),
    "schedule", readonly(&NNGN::schedule),
    "graphics", property([](const NNGN &n) { return n.graphics.get(); }),
    "fps", readonly(&NNGN::fps),
    "socket", readonly(&NNGN::socket),
    "lua", readonly(&NNGN::lua),
    "set_graphics", &NNGN::set_graphics,
    "exit", &NNGN::exit,
    "die", &NNGN::die)

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
    const auto exit = nngn::make_scoped([&s = nngn.schedule] { s.exit(); });
    return nngn::Platform::loop([]() { return p_nngn->loop(); });
}
