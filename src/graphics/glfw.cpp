#include "os/platform.h"
#include "utils/log.h"

#include "glfw.h"

#ifndef NNGN_PLATFORM_HAS_GLFW

namespace nngn {

template<> std::unique_ptr<Graphics> graphics_create_backend
        <Graphics::Backend::GLFW_BACKEND>(const void*) {
    Log::l() << "compiled without GLFW support\n";
    return {};
}

}

#else

#include <GLFW/glfw3.h>

#include "utils/ranges.h"

static_assert(
    nngn::is_adjacent(
        nngn::owning_view{std::array{
            GLFW_CURSOR_NORMAL,
            GLFW_CURSOR_HIDDEN,
            GLFW_CURSOR_DISABLED,
        }}));

namespace nngn {

template<>
std::unique_ptr<Graphics> graphics_create_backend<
    Graphics::Backend::GLFW_BACKEND
>(const void *params)
{
    NNGN_LOG_CONTEXT_F();
    if(params) {
        Log::l() << "no parameters allowed\n";
        return {};
    }
    return std::make_unique<GLFWBackend>();
}

GLFWBackend::~GLFWBackend(void) {
    if(this->w)
        glfwDestroyWindow(this->w);
    glfwTerminate();
}

bool GLFWBackend::init() {
    NNGN_LOG_CONTEXT_CF(GLFWBackend);
    glfwSetErrorCallback([](int, const char *description)
        { Log::l() << "glfw: " << description << std::endl; });
    if(!glfwInit())
        return false;
    constexpr int initial_width = 800, initial_height = 600;
    if(!(this->w = glfwCreateWindow(
            initial_width, initial_height, "", nullptr, nullptr)))
        return Log::l() << "failed to initialize GLFW window\n", false;
    this->callback_data.p = this;
    glfwSetWindowUserPointer(this->w, &this->callback_data);
    glfwSetFramebufferSizeCallback(
        this->w, [](GLFWwindow *cw, int width, int height) {
            static_cast<const CallbackData*>(glfwGetWindowUserPointer(cw))
                ->p->resize(width, height);
    });
    glfwShowWindow(this->w);
    return true;
}

bool GLFWBackend::window_closed(void) const {
    return glfwWindowShouldClose(this->w);
}

void GLFWBackend::set_swap_interval(int i) {
    glfwSwapInterval(this->m_swap_interval = i);
}

void GLFWBackend::set_window_title(const char *t) {
    glfwSetWindowTitle(this->w, t);
}

void GLFWBackend::set_cursor_mode(CursorMode m) {
    glfwSetInputMode(
        this->w, GLFW_CURSOR,
        GLFW_CURSOR_NORMAL + static_cast<int>(m));
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

void GLFWBackend::poll_events(void) const { glfwPollEvents(); }
bool GLFWBackend::render(void) { glfwSwapBuffers(this->w); return true; }

}

#endif
