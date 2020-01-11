/**
 * \dir src
 * \brief Main source directory (C++20).
 *
 * Only a minimal amount of core source lives directly under this directory.
 * Most code is placed under modules in each subdirectory (see the documentation
 * of each of them for details).  Dependencies between modules are kept to a
 * minimum and with few exceptional cases the dependency graph is a DAG.
 */
#include "graphics/graphics.h"
#include "math/math.h"
#include "os/platform.h"
#include "os/socket.h"
#include "timing/fps.h"
#include "timing/timing.h"
#include "utils/flags.h"
#include "utils/log.h"

namespace {

using namespace std::string_view_literals;

struct NNGN {
    enum Flag : uint8_t {
        EXIT = 1u << 0u,
    };
    nngn::Flags<Flag> flags = {};
    nngn::Math math = {};
    nngn::Timing timing = {};
    std::unique_ptr<nngn::Graphics> graphics = {};
    nngn::FPS fps = {};
    nngn::Socket socket = {};
    bool init(int argc, const char *const *argv);
    int loop(void);
} *p_nngn;

bool NNGN::init(int argc, const char *const *argv) {
    NNGN_LOG_CONTEXT_CF(NNGN);
    this->math.init();
    if(!nngn::Platform::init(argc, argv))
        return false;
    if(!this->socket.init("sock"))
        return false;
    this->graphics = nngn::graphics_create_backend
        <nngn::Graphics::Backend::GLFW_BACKEND>();
    if(!this->graphics)
        this->graphics = nngn::graphics_create_backend
            <nngn::Graphics::Backend::PSEUDOGRAPH>();
    if(!this->graphics->init())
        return false;
    this->fps.init(nngn::Timing::clock::now());
    return true;
}

int NNGN::loop(void) {
    if(this->flags.is_set(Flag::EXIT) || this->graphics->window_closed())
        return 0;
    this->timing.update();
    this->graphics->poll_events();
    bool ok = true;
    ok = ok && this->socket.process([&f = this->flags](auto s)
        { if(s == "exit\n\0"sv) f.set(Flag::EXIT); });
    ok = ok && this->graphics->render();
    if(!ok)
        return 1;
    this->fps.frame(nngn::Timing::clock::now());
    this->graphics->set_window_title(this->fps.to_string().c_str());
    return -1;
}

}

int main(int argc, const char *const *argv) {
    NNGN nngn;
    p_nngn = &nngn;
    if(!nngn.init(argc, argv))
        return 1;
    return nngn::Platform::loop([]() { return p_nngn->loop(); });
}
