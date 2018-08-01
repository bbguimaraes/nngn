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

#include "entity.h"

#include "graphics/graphics.h"
#include "graphics/texture.h"
#include "input/input.h"
#include "input/mouse.h"
#include "lua/alloc.h"
#include "lua/function.h"
#include "lua/iter.h"
#include "lua/register.h"
#include "lua/table.h"
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

NNGN_LUA_DECLARE_USER_TYPE(Entities, "Entities")
NNGN_LUA_DECLARE_USER_TYPE(Entity, "Entity")
NNGN_LUA_DECLARE_USER_TYPE(nngn::Math, "Math")
NNGN_LUA_DECLARE_USER_TYPE(nngn::Timing, "Timing")
NNGN_LUA_DECLARE_USER_TYPE(nngn::Schedule, "Schedule")
NNGN_LUA_DECLARE_USER_TYPE(nngn::Graphics, "Graphics")
NNGN_LUA_DECLARE_USER_TYPE(nngn::Graphics::Parameters, "Graphics::Parameters")
NNGN_LUA_DECLARE_USER_TYPE(nngn::FPS, "FPS")
NNGN_LUA_DECLARE_USER_TYPE(nngn::Socket, "Socket")
NNGN_LUA_DECLARE_USER_TYPE(nngn::lua::state, "state")
NNGN_LUA_DECLARE_USER_TYPE(nngn::Input, "Input")
NNGN_LUA_DECLARE_USER_TYPE(nngn::MouseInput, "MouseInput")
NNGN_LUA_DECLARE_USER_TYPE(nngn::Renderers, "Renderers")
NNGN_LUA_DECLARE_USER_TYPE(nngn::Textures, "Textures")

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
    nngn::Textures textures = {};
    bool init(int argc, const char *const *argv);
    bool set_graphics(nngn::Graphics::Backend b, const void *params);
    int loop(void);
    void remove_entity(Entity *e);
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
    nngn::Profile::init();
    if(!this->lua.init(&this->lua_alloc))
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
    this->schedule.init(&this->timing);
    this->input.input.init(this->lua);
    this->input.mouse.init(this->lua);
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
        &this->input.mouse, [](void *p, nngn::dvec2 pos) {
            static_cast<nngn::MouseInput*>(p)->move_callback(pos);
        });
    const bool ret = this->renderers.set_graphics(g.get());
    this->textures.set_graphics(g.get());
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

void register_nngn(nngn::lua::table &&t) {
    using nngn::lua::accessor;
    t["math"] = accessor<&NNGN::math>;
    t["timing"] = accessor<&NNGN::timing>;
    t["schedule"] = accessor<&NNGN::schedule>;
    t["graphics"] = [](NNGN &nngn) { return nngn.graphics.get(); };
    t["fps"] = accessor<&NNGN::fps>;
    t["socket"] = accessor<&NNGN::socket>;
    t["lua"] = accessor<&NNGN::lua>;
    t["input"] = [](NNGN &nngn) { return &nngn.input.input; };
    t["mouse_input"] = [](NNGN &nngn) { return &nngn.input.mouse; };
    t["renderers"] = accessor<&NNGN::renderers>;
    t["entities"] = accessor<&NNGN::entities>;
    t["textures"] = accessor<&NNGN::textures>;
    t["set_graphics"] = &NNGN::set_graphics;
    t["remove_entity"] = &NNGN::remove_entity;
    t["remove_entities"] = [](NNGN &nngn, nngn::lua::table_view es) {
        for(auto [_, x] : ipairs(es))
            nngn.remove_entity(x.get<Entity*>());
    };
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
    const auto exit = nngn::make_scoped([&s = nngn.schedule] { s.exit(); });
    return nngn::Platform::loop([]() { return p_nngn->loop(); });
}
