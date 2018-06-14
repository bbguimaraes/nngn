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
#include "entity.h"

#include "graphics/graphics.h"
#include "input/input.h"
#include "input/mouse.h"
#include "lua/alloc.h"
#include "lua/state.h"
#include "math/math.h"
#include "os/platform.h"
#include "os/socket.h"
#include "render/render.h"
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
    struct Input {
        nngn::Input input;
        nngn::MouseInput mouse;
        struct {
            nngn::Input::Source *graphics = {};
        } sources;
    };
    nngn::Flags<Flag> flags = {};
    nngn::Math math = {};
    nngn::Timing timing = {};
    nngn::Schedule schedule = {};
    std::unique_ptr<nngn::Graphics> graphics = {};
    nngn::FPS fps = {};
    nngn::Socket socket = {};
    nngn::lua::state lua = {};
    nngn::lua::alloc_info lua_alloc = {};
    Input input = {};
    nngn::Renderers renderers = {};
    Entities entities = {};
    bool init(int argc, const char *const *argv);
    bool set_graphics(
        nngn::Graphics::Backend b, std::optional<const void*> params);
    int loop(void);
    void remove_entity(Entity *e);
    void exit(void) { this->flags |= Flag::EXIT; }
    void die(void) { this->flags |= Flag::ERROR; }
} *p_nngn;

bool NNGN::init(int argc, const char *const *argv) {
    NNGN_LOG_CONTEXT_CF(NNGN);
    this->math.init();
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
    this->input.input.init(this->lua);
    this->input.mouse.init(this->lua);
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
    {
        auto &i = this->input;
        auto src = nngn::input_graphics_source(g.get());
        if(auto *p = std::exchange(i.sources.graphics, src.get()))
            i.input.remove_source(p);
        i.input.add_source(std::move(src));
    }
    g->set_mouse_button_callback(
        &this->input.mouse, [](void *p, int button, int action, int mods) {
            static_cast<nngn::MouseInput*>(p)
                ->button_callback(button, action, mods);
    });
    g->set_mouse_move_callback(
        &this->input.mouse, [](void *p, nngn::dvec2 pos)
            { static_cast<nngn::MouseInput*>(p)->move_callback(pos); });
    bool ret = this->renderers.set_graphics(g.get());
    this->graphics = std::move(g);
    return ret;
}

int NNGN::loop(void) {
    if(this->flags.is_set(Flag::ERROR))
        return 1;
    if(this->flags.is_set(Flag::EXIT) || this->graphics->window_closed())
        return 0;
    this->timing.update();
    const bool ok = this->input.input.update()
        && this->schedule.update()
        && this->socket.process([&l = this->lua](auto s) { l.dostring(s); })
        && this->renderers.update();
    if(!ok)
        return 1;
    this->entities.clear_flags();
    if(!this->graphics->render() || !this->graphics->vsync())
        return 1;
    nngn::Profile::swap();
    this->fps.frame(nngn::Timing::clock::now());
    this->graphics->set_window_title(this->fps.to_string().c_str());
    return -1;
}

void NNGN::remove_entity(Entity *e) {
    assert(e);
    if(e->renderer)
        this->renderers.remove(e->renderer);
    this->entities.remove(e);
}

using nngn::lua::property, nngn::lua::readonly;

NNGN_LUA_PROXY(NNGN,
    "math", readonly(&NNGN::math),
    "timing", readonly(&NNGN::timing),
    "schedule", readonly(&NNGN::schedule),
    "graphics", property([](const NNGN &n) { return n.graphics.get(); }),
    "fps", readonly(&NNGN::fps),
    "socket", readonly(&NNGN::socket),
    "lua", readonly(&NNGN::lua),
    "input", property([](const NNGN &nngn) { return &nngn.input.input; }),
    "mouse_input", property([](const NNGN &nngn) { return &nngn.input.mouse; }),
    "renderers", readonly(&NNGN::renderers),
    "entities", readonly(&NNGN::entities),
    "set_graphics", &NNGN::set_graphics,
    "remove_entity", &NNGN::remove_entity,
    "remove_entity_v", [](NNGN &nngn, nngn::lua::table_view t) {
        for(lua_Integer i = 1, n = t.size(); i <= n; ++i)
            nngn.remove_entity(t[i]);
    },
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
