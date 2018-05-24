#ifndef NNGN_GRAPHICS_GLFW_H
#define NNGN_GRAPHICS_GLFW_H

#include "utils/utils.h"

#include "graphics.h"

namespace nngn {

class GLFWBackend final : public Graphics {
    int m_swap_interval = 1;
    GLFWwindow *w = nullptr;
    struct CallbackData {
        Graphics *p = {};
        key_callback_f key_cb = {};
        mouse_button_callback_f mouse_button_cb = {};
        mouse_move_callback_f mouse_move_cb = {};
        void *key_cb_data = {};
        void *mouse_button_cb_data = {};
        void *mouse_move_cb_data = {};
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
    void set_cursor_mode(CursorMode m) final;
    void set_key_callback(void *data, key_callback_f f) override;
    void set_mouse_button_callback(
        void *data, mouse_button_callback_f f) override;
    void set_mouse_move_callback(
        void *data, mouse_move_callback_f f) override;
    void resize(int, int) final {}
    void poll_events(void) const final;
    bool render(void) final;
};

}

#endif
