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
#include <string>
#include <ElysianLua/elysian_lua_function.hpp>

#include "entity.h"

#include "audio/audio.h"
#include "collision/collision.h"
#include "compute/compute.h"
#include "font/font.h"
#include "font/textbox.h"
#include "graphics/graphics.h"
#include "graphics/texture.h"
#include "input/input.h"
#include "input/mouse.h"
#include "lua/alloc.h"
#include "lua/state.h"
#include "math/camera.h"
#include "math/math.h"
#include "os/platform.h"
#include "os/socket.h"
#include "render/animation.h"
#include "render/grid.h"
#include "render/light.h"
#include "render/map.h"
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
    std::unique_ptr<nngn::Compute> compute = {};
    std::unique_ptr<nngn::Graphics> graphics = {};
    nngn::FPS fps = {};
    nngn::Socket socket = {};
    nngn::lua::state lua = {};
    nngn::lua::alloc_info lua_alloc = {};
    Input input = {};
    nngn::Fonts fonts = {};
    nngn::Grid grid = {};
    nngn::Textbox textbox = {};
    nngn::Camera camera = {};
    nngn::Renderers renderers = {};
    nngn::Animations animations = {};
    nngn::Colliders colliders = {};
    Entities entities = {};
    nngn::Audio audio = {};
    nngn::Textures textures = {};
    nngn::Lighting lighting = {};
    nngn::Map map = {};
    bool init(int argc, const char *const *argv);
    bool set_compute(
        nngn::Compute::Backend b, std::optional<const void*> params);
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
    elysian::lua::LuaVM::initialize(this->lua);
    nngn::lua::static_register::register_all(this->lua);
    const auto L = nngn::lua::global_table{this->lua};
    L["nngn"] = nngn::lua::sol_user_type{this};
    L["deref"] = +[](lua_State *L_) {
        const auto u = *static_cast<std::uintptr_t*>(lua_touserdata(L_, 1));
        lua_pushinteger(L_, static_cast<lua_Integer>(u));
        return 1;
    };
    L["log"] = +[](lua_State *L_) {
        auto &l = nngn::Log::l();
        for(int i = 1, t = lua_gettop(L_); i <= t; ++i)
            l << lua_tostring(L_, i);
        return 0;
    };
    this->schedule.init(&this->timing);
    this->input.input.init(this->lua);
    this->input.mouse.init(this->lua);
    if(!this->fonts.init())
        return false;
    this->renderers.init(
        &this->textures, &this->fonts, &this->textbox, &this->grid,
        &this->colliders, &this->lighting, &this->map);
    this->animations.init(&this->math);
    this->textbox.init(&this->fonts);
    if(!this->audio.init(&this->math, 44100))
        return false;
    this->lighting.init(&this->math);
    this->map.init(&this->textures);
    if(!(argc < 2
        ? this->lua.dofile("src/lua/all.lua")
        : std::ranges::all_of(
            nngn::Platform::argv.subspan(1),
            NNGN_BIND_MEM_FN(&this->lua, doarg))
    ))
        return false;
    this->fps.init(nngn::Timing::clock::now());
    if(!this->compute)
        this->set_compute(nngn::Compute::Backend::PSEUDOCOMP, {});
    if(!this->graphics)
        this->set_graphics(nngn::Graphics::Backend::PSEUDOGRAPH, {});
    if(!this->colliders.has_backend())
        this->colliders.set_backend(nngn::Colliders::native_backend());
    return true;
}

bool NNGN::set_compute(
        nngn::Compute::Backend b, std::optional<const void*> params) {
    this->compute = nngn::Compute::create(
        b, params ? static_cast<const uintptr_t*>(*params) + 1 : nullptr);
    auto *c = this->compute.get();
    return c && c->init();
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
    const bool ret = this->grid.set_graphics(g.get())
        && this->map.set_graphics(g.get())
        && this->renderers.set_graphics(g.get());
    this->fonts.graphics = g.get();
    this->textures.set_graphics(g.get());
    g->set_size_callback(
        &this->camera, [](void *p, auto s)
            { static_cast<nngn::Camera*>(p)->set_screen(s); });
    g->set_camera({
        &this->camera.flags.t, &this->camera.screen,
        &this->camera.proj, &this->camera.hud_proj, &this->camera.view});
    this->camera.set_screen(g->window_size());
    g->set_lighting({
        &this->lighting.ubo(),
        &this->lighting.dir_proj(), &this->lighting.point_proj(),
        &this->lighting.dir_view(0), &this->lighting.point_view(0, 0)});
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
        && this->socket.process([&l = this->lua](auto s) { l.dostring(s); });
    if(!ok)
        return 1;
    this->entities.update(this->timing);
    this->animations.update(this->timing);
    if(!this->colliders.check_collisions(this->timing))
        return 1;
    this->colliders.resolve_collisions();
    this->colliders.lua_on_collision(this->lua);
    this->entities.update_children();
    if(this->camera.flags & nngn::Camera::Flag::SCREEN_UPDATED)
        this->textbox.set_screen_updated();
    if(this->camera.update(this->timing)) {
        this->graphics->set_camera_updated();
        this->lighting.update_view(this->camera.eye());
    }
    if(this->textbox.update(this->timing))
        this->textbox.update_size(this->camera.screen);
    if(!this->renderers.update())
        return 1;
    this->entities.clear_flags();
    this->textbox.clear_updated();
    if(this->lighting.update(this->timing))
        this->graphics->set_lighting_updated();
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
    if(e->anim)
        this->animations.remove(e->anim);
    if(e->collider)
        this->colliders.remove(e->collider);
    if(e->light)
        this->lighting.remove_light(e->light);
    this->entities.remove(e);
}

using nngn::lua::property, nngn::lua::readonly;

NNGN_LUA_PROXY(NNGN,
    "math", readonly(&NNGN::math),
    "timing", readonly(&NNGN::timing),
    "schedule", readonly(&NNGN::schedule),
    "compute", property([](const NNGN &n) { return n.compute.get(); }),
    "graphics", property([](const NNGN &n) { return n.graphics.get(); }),
    "fps", readonly(&NNGN::fps),
    "socket", readonly(&NNGN::socket),
    "lua", readonly(&NNGN::lua),
    "input", property([](const NNGN &nngn) { return &nngn.input.input; }),
    "mouse_input", property([](const NNGN &nngn) { return &nngn.input.mouse; }),
    "fonts", readonly(&NNGN::fonts),
    "grid", readonly(&NNGN::grid),
    "textbox", readonly(&NNGN::textbox),
    "camera", readonly(&NNGN::camera),
    "renderers", readonly(&NNGN::renderers),
    "animations", readonly(&NNGN::animations),
    "colliders", readonly(&NNGN::colliders),
    "entities", readonly(&NNGN::entities),
    "audio", [](lua_State *L) {
        lua_pushlightuserdata(
            L, &(*static_cast<NNGN**>(lua_touserdata(L, 1)))->audio);
        lua_getglobal(L, "Audio");
        lua_setmetatable(L, -2);
        return 1;
    },
    "textures", readonly(&NNGN::textures),
    "lighting", readonly(&NNGN::lighting),
    "map", readonly(&NNGN::map),
    "set_compute", &NNGN::set_compute,
    "set_graphics", &NNGN::set_graphics,
    "remove_entity", &NNGN::remove_entity,
    "remove_entity_v", [](NNGN &nngn, nngn::lua::table_view t) {
        for(lua_Integer i = 1, n = t.size(); i <= n; ++i)
            nngn.remove_entity(
                static_cast<nngn::lua::sol_user_type<Entity*>>(t[i]));
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
