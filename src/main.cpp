#include "graphics/graphics.h"
#include "os/platform.h"
#include "utils/log.h"

namespace {

struct NNGN {
    std::unique_ptr<nngn::Graphics> graphics = {};
    bool init(int argc, const char *const *argv);
    int loop(void) const;
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

int NNGN::loop(void) const {
    if(this->graphics->window_closed())
        return 0;
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
