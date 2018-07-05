#include "input.h"

#include "graphics/graphics.h"
#include "os/platform.h"
#include "utils/utils.h"

#include "mouse.h"

#ifdef NNGN_PLATFORM_HAS_GLFW
#include <GLFW/glfw3.h>
#endif

using nngn::i32;

namespace {

class GraphicsSource : public nngn::Input::Source {
    using Action = nngn::Input::Action;
    using Modifier = nngn::Input::Modifier;
    nngn::Graphics *graphics = {};
    std::vector<std::tuple<int, Action, Modifier>> events = {};
public:
    NNGN_MOVE_ONLY(GraphicsSource)
    GraphicsSource(nngn::Graphics *g);
    ~GraphicsSource(void) override = default;
    bool update(nngn::Input*) override;
    void get_keys(std::span<i32> keys) const override;
};

GraphicsSource::GraphicsSource(nngn::Graphics *g) : graphics(g) {
    g->set_key_callback(
        this, [](void *p, int key, int, int action, int mods) {
            static_cast<GraphicsSource*>(p)->events.emplace_back(
                key, static_cast<nngn::Input::Action>(action),
                static_cast<nngn::Input::Modifier>(mods));
    });
}

bool GraphicsSource::update(nngn::Input *input) {
    this->graphics->poll_events();
    for(const auto &x : this->events) {
        const auto [key, action, mod] = x;
        input->key_callback(key, action, mod);
    }
    this->events.clear();
    return true;
}

void GraphicsSource::get_keys(std::span<i32> keys) const {
    this->graphics->get_keys(keys.size(), keys.data());
}

}

namespace nngn {

#ifdef NNGN_PLATFORM_HAS_GLFW
static_assert(Input::MOD_SHIFT == GLFW_MOD_SHIFT);
static_assert(Input::MOD_CTRL == GLFW_MOD_CONTROL);
static_assert(Input::MOD_ALT == GLFW_MOD_ALT);
static_assert(Input::KEY_PRESS == GLFW_PRESS);
static_assert(Input::KEY_RELEASE == GLFW_RELEASE);
static_assert(Input::KEY_ESC == GLFW_KEY_ESCAPE);
static_assert(Input::KEY_ENTER == GLFW_KEY_ENTER);
static_assert(Input::KEY_TAB == GLFW_KEY_TAB);
static_assert(Input::KEY_RIGHT == GLFW_KEY_RIGHT);
static_assert(Input::KEY_LEFT == GLFW_KEY_LEFT);
static_assert(Input::KEY_DOWN == GLFW_KEY_DOWN);
static_assert(Input::KEY_UP == GLFW_KEY_UP);
static_assert(Input::KEY_PAGE_UP == GLFW_KEY_PAGE_UP);
static_assert(Input::KEY_PAGE_DOWN == GLFW_KEY_PAGE_DOWN);
static_assert(Input::KEY_MAX == GLFW_KEY_LAST);
static_assert(
    static_cast<int>(MouseInput::Action::PRESS) == GLFW_PRESS);
static_assert(
    static_cast<int>(MouseInput::Action::RELEASE) == GLFW_RELEASE);
#endif

std::unique_ptr<Input::Source> input_graphics_source(Graphics *g) {
    return std::make_unique<GraphicsSource>(g);
}

}
