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
#include "os/platform.h"
#include "timing/timing.h"
#include "utils/log.h"

namespace {

struct NNGN {
    nngn::Timing timing = {};
    std::unique_ptr<nngn::Graphics> graphics = {};
    bool init(int argc, const char *const *argv);
    int loop(void);
} *p_nngn;

bool NNGN::init(int argc, const char *const *argv) {
    NNGN_LOG_CONTEXT_CF(NNGN);
    if(!nngn::Platform::init(argc, argv))
        return false;
    this->graphics = nngn::graphics_create_backend
        <nngn::Graphics::Backend::GLFW_BACKEND>();
    if(!this->graphics)
        this->graphics = nngn::graphics_create_backend
            <nngn::Graphics::Backend::PSEUDOGRAPH>();
    return this->graphics->init();
}

int NNGN::loop(void) {
    if(this->graphics->window_closed())
        return 0;
    this->timing.update();
    this->graphics->poll_events();
    if(!this->graphics->render())
        return 1;
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
