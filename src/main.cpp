#include <sol/state_view.hpp>

#include "luastate.h"

#include "graphics/graphics.h"
#include "input/input.h"
#include "input/mouse.h"
#include "math/math.h"
#include "os/platform.h"
#include "os/socket.h"
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
    bool init(int argc, const char *const *argv);
    bool set_graphics(
        nngn::Graphics::Backend b, std::optional<const void*> params);
    int loop();
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
    this->graphics = std::move(g);
    return true;
}

int NNGN::loop() {
    if(this->flags.is_set(Flag::ERROR))
        return 1;
    if(this->flags.is_set(Flag::EXIT) || this->graphics->window_closed())
        return 0;
    this->timing.update();
    bool ok = true;
    ok = ok && this->input.input.update();
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
