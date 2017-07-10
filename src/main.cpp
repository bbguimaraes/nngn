#include <sol/state_view.hpp>

#include "entity.h"
#include "luastate.h"
#include "player.h"

#include "font/font.h"
#include "font/textbox.h"
#include "graphics/graphics.h"
#include "graphics/texture.h"
#include "input/input.h"
#include "input/mouse.h"
#include "math/camera.h"
#include "math/math.h"
#include "os/platform.h"
#include "os/socket.h"
#include "render/animation.h"
#include "render/grid.h"
#include "render/render.h"
#include "timing/fps.h"
#include "timing/profile.h"
#include "timing/schedule.h"
#include "timing/timing.h"
#include "utils/flags.h"
#include "utils/log.h"

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
    LuaState lua = {};
    Input input = {};
    nngn::Fonts fonts = {};
    nngn::Grid grid = {};
    nngn::Textbox textbox = {};
    nngn::Camera camera = {};
    nngn::Renderers renderers = {};
    nngn::Animations animations = {};
    Entities entities = {};
    Players players = {};
    nngn::Textures textures = {};
    bool init(int argc, const char *const *argv);
    bool set_graphics(
        nngn::Graphics::Backend b, std::optional<const void*> params);
    int loop();
    void remove_entity(Entity *e);
    void exit() { this->flags |= Flag::EXIT; }
    void die() { this->flags |= Flag::ERROR; }
} *p_nngn;

bool NNGN::init(int argc, const char *const *argv) {
    NNGN_LOG_CONTEXT_CF(NNGN);
    this->math.init();
    if(!nngn::Platform::init(argc, argv))
        return false;
    nngn::Profile::init();
    if(!this->lua.init())
        return false;
    auto sol = sol::state_view(this->lua.L);
    sol["nngn"] = this;
    sol["deref"] = [](void *p) { return *static_cast<uintptr_t*>(p); };
    this->schedule.init(&this->timing);
    this->input.input.init(this->lua.L);
    this->input.mouse.init(this->lua.L);
    if(!this->fonts.init())
        return false;
    this->renderers.init(
        &this->textures, &this->fonts, &this->textbox, &this->grid);
    this->animations.init(&this->math);
    this->textbox.init(&this->fonts);
    if(!(argc < 2
        ? this->lua.dofile("src/lua/all.lua")
        : std::all_of(
            argv + 1, argv + argc,
            [&l = this->lua](auto x) { return l.dofile(x); })))
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
    const bool ret = this->grid.set_graphics(g.get())
        && this->renderers.set_graphics(g.get());
    this->fonts.graphics = g.get();
    this->textures.set_graphics(g.get());
    g->set_size_callback(
        &this->camera, [](void *p, auto s)
            { static_cast<nngn::Camera*>(p)->set_screen(s); });
    g->set_camera({
        &this->camera.screen, &this->camera.proj, &this->camera.hud_proj,
        &this->camera.view});
    this->camera.set_screen(g->window_size());
    this->graphics = std::move(g);
    return ret;
}

int NNGN::loop() {
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
    if(this->camera.flags & nngn::Camera::Flag::SCREEN_UPDATED)
        this->textbox.flags.set(nngn::Textbox::Flag::SCREEN_UPDATED);
    if(this->camera.update(this->timing))
        this->graphics->set_camera_updated();
    if(this->textbox.update(this->timing))
        this->textbox.update_size(this->camera.screen);
    if(!this->renderers.update())
        return 1;
    this->entities.clear_flags();
    this->textbox.clear_updated();
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
    this->entities.remove(e);
}

NNGN_LUA_PROXY(NNGN,
    "math", sol::readonly(&NNGN::math),
    "timing", sol::readonly(&NNGN::timing),
    "schedule", sol::readonly(&NNGN::schedule),
    "graphics", sol::property(
        [](const NNGN &nngn) { return nngn.graphics.get(); }),
    "fps", sol::readonly(&NNGN::fps),
    "socket", sol::readonly(&NNGN::socket),
    "lua", sol::readonly(&NNGN::lua),
    "input", sol::property([](const NNGN &nngn) { return &nngn.input.input; }),
    "mouse_input", sol::property(
        [](const NNGN &nngn) { return &nngn.input.mouse; }),
    "fonts", sol::readonly(&NNGN::fonts),
    "grid", sol::readonly(&NNGN::grid),
    "textbox", sol::readonly(&NNGN::textbox),
    "camera", sol::readonly(&NNGN::camera),
    "renderers", sol::readonly(&NNGN::renderers),
    "animations", sol::readonly(&NNGN::animations),
    "entities", sol::readonly(&NNGN::entities),
    "players", sol::readonly(&NNGN::players),
    "textures", sol::readonly(&NNGN::textures),
    "set_graphics", &NNGN::set_graphics,
    "remove_entity", &NNGN::remove_entity,
    "remove_entity_v", [](NNGN &nngn, const sol::table &t) {
        for(size_t i = 1, n = t.size(); i <= n; ++i)
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
