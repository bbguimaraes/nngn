#ifndef NNGN_GRAPHICS_GLFW_H
#define NNGN_GRAPHICS_GLFW_H

#include "utils/utils.h"

#include "graphics.h"

namespace nngn {

class GLFWBackend final : public Graphics {
    int m_swap_interval = 1;
    GLFWwindow *w = nullptr;
    struct CallbackData {
        Graphics *p = nullptr;
    } callback_data = {};
public:
    NNGN_MOVE_ONLY(GLFWBackend)
    GLFWBackend(void) = default;
    ~GLFWBackend(void) final;
    bool init(void) final;
    bool window_closed(void) const final;
    int swap_interval(void) const final { return this->m_swap_interval; }
    void set_swap_interval(int i) final;
    void set_window_title(const char *t) final;
    void resize(int, int) final {}
    void poll_events(void) const final;
    bool render(void) final;
};

}

#endif
