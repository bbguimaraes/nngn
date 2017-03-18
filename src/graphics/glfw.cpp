#include "os/platform.h"
#include "utils/log.h"

#include "glfw.h"

#ifndef NNGN_PLATFORM_HAS_GLFW

namespace nngn {

template<> std::unique_ptr<Graphics> graphics_create_backend
        <Graphics::Backend::GLFW_BACKEND>() {
    Log::l() << "compiled without GLFW support\n";
    return {};
}

}

#else

#include <GLFW/glfw3.h>

namespace nngn {

template<> std::unique_ptr<Graphics> graphics_create_backend
        <Graphics::Backend::GLFW_BACKEND>()
    { return std::make_unique<GLFWBackend>(); }

GLFWBackend::~GLFWBackend() {
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

bool GLFWBackend::window_closed() const
    { return glfwWindowShouldClose(this->w); }
void GLFWBackend::set_swap_interval(int i)
    { glfwSwapInterval(this->m_swap_interval = i); }
void GLFWBackend::poll_events() const { glfwPollEvents(); }
bool GLFWBackend::render() { glfwSwapBuffers(this->w); return true; }

}

#endif
