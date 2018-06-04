#include "os/platform.h"
#include "utils/log.h"

#ifdef NNGN_PLATFORM_HAS_GLFW

#include <algorithm>

#include <GLFW/glfw3.h>

#include "utils/ranges.h"

#include "glfw.h"

static_assert(
    nngn::is_adjacent(
        nngn::owning_view{std::array{
            GLFW_CURSOR_NORMAL,
            GLFW_CURSOR_HIDDEN,
            GLFW_CURSOR_DISABLED,
        }}));

#include "glfw.h"

namespace nngn {

GLFWBackend::~GLFWBackend() {
    if(this->w)
        glfwDestroyWindow(this->w);
    glfwTerminate();
}

bool GLFWBackend::init_glfw(void) const {
    NNGN_LOG_CONTEXT_CF(GLFWBackend);
    if(this->params.flags.is_set(Parameters::Flag::DEBUG))
        glfwSetErrorCallback([](int, const char *description)
            { nngn::Log::l() << "glfw: " << description << std::endl; });
    return glfwInit();
}

bool GLFWBackend::create_window() {
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    constexpr int initial_width = 800, initial_height = 600;
    if(!(this->w = glfwCreateWindow(
            initial_width, initial_height, "", nullptr, nullptr)))
        return Log::l() << "failed to initialize GLFW window\n", false;
    this->callback_data.p = this;
    glfwSetWindowUserPointer(this->w, &this->callback_data);
    this->set_size_callback(nullptr, nullptr);
    return true;
}

bool GLFWBackend::window_closed() const
    { return glfwWindowShouldClose(this->w); }

uvec2 GLFWBackend::window_size() const {
    ivec2 ret;
    glfwGetWindowSize(this->w, &ret.x, &ret.y);
    return static_cast<uvec2>(ret);
}

void GLFWBackend::get_keys(size_t n, int32_t *keys) const {
    std::transform(
        keys, keys + n, keys,
        [w = this->w](auto x) { return static_cast<char>(glfwGetKey(w, x)); });
}

void GLFWBackend::set_swap_interval(int i)
    { glfwSwapInterval(this->m_swap_interval = i); }
void GLFWBackend::set_window_title(const char *t)
    { glfwSetWindowTitle(this->w, t); }
void GLFWBackend::set_cursor_mode(CursorMode m) {
    glfwSetInputMode(
        this->w, GLFW_CURSOR,
        GLFW_CURSOR_NORMAL + static_cast<int>(m));
}

void GLFWBackend::set_size_callback(void *data, size_callback_f f) {
    this->callback_data.size_cb_data = data;
    this->callback_data.size_cb = f;
    glfwSetFramebufferSizeCallback(
        this->w, [](GLFWwindow *cw, int width, int height) {
            const auto *d = static_cast<const CallbackData*>(
                glfwGetWindowUserPointer(cw));
            if(const auto c = d->size_cb)
                c(d->size_cb_data, static_cast<uvec2>(ivec2{width, height}));
            d->p->resize(width, height);
    });
}

void GLFWBackend::set_key_callback(void *data, key_callback_f f) {
    this->callback_data.key_cb_data = data;
    this->callback_data.key_cb = f;
    glfwSetKeyCallback(
        this->w, [](GLFWwindow *cw, int key, int scan, int action, int mods) {
            auto *d = static_cast<CallbackData*>(glfwGetWindowUserPointer(cw));
            if(d->key_cb)
                d->key_cb(d->key_cb_data, key, scan, action, mods);
    });
}

void GLFWBackend::set_mouse_button_callback(
    void *data, mouse_button_callback_f f
) {
    this->callback_data.mouse_button_cb_data = data;
    this->callback_data.mouse_button_cb = f;
    glfwSetMouseButtonCallback(
        this->w, [](GLFWwindow *cw, int button, int action, int mods) {
            auto *d = static_cast<CallbackData*>(glfwGetWindowUserPointer(cw));
            if(d->mouse_button_cb)
                d->mouse_button_cb(
                    d->mouse_button_cb_data, button, action, mods);
    });
}

void GLFWBackend::set_mouse_move_callback(
    void *data, mouse_move_callback_f f
) {
    this->callback_data.mouse_move_cb_data = data;
    this->callback_data.mouse_move_cb = f;
    glfwSetCursorPosCallback(
        this->w, [](GLFWwindow *cw, double x, double y) {
            auto *d = static_cast<CallbackData*>(glfwGetWindowUserPointer(cw));
            if(d->mouse_move_cb)
                d->mouse_move_cb(d->mouse_move_cb_data, {x, y});
    });
}

void GLFWBackend::poll_events() const { glfwPollEvents(); }
bool GLFWBackend::render() { glfwSwapBuffers(this->w); return true; }

}

#endif
