#ifndef NNGN_GRAPHICS_GLFW_H
#define NNGN_GRAPHICS_GLFW_H

#include "graphics.h"

namespace nngn {

class GLFWBackend final : public Graphics {
    int m_swap_interval = 1;
    GLFWwindow *w = nullptr;
    struct CallbackData {
        Graphics *p = nullptr;
    } callback_data = {};
public:
    NNGN_NO_COPY(GLFWBackend)
    GLFWBackend() = default;
    ~GLFWBackend() final;
    bool init() final;
    bool window_closed() const final;
    int swap_interval() const final { return this->m_swap_interval; }
    void set_swap_interval(int i) final;
    void set_window_title(const char *t) final;
    void resize(int, int) final {}
    void poll_events() const final;
    bool render() final;
};

}

#endif
