#ifndef NNGN_GRAPHICS_GLFW_H
#define NNGN_GRAPHICS_GLFW_H

#include "utils/utils.h"

#include "graphics.h"

namespace nngn {

class GLFWBackend : public Graphics {
    struct CallbackData {
        Graphics *p = {};
        size_callback_f size_cb = {};
        key_callback_f key_cb = {};
        mouse_button_callback_f mouse_button_cb = {};
        mouse_move_callback_f mouse_move_cb = {};
        void *size_cb_data = {};
        void *key_cb_data = {};
        void *mouse_button_cb_data = {};
        void *mouse_move_cb_data = {};
    } callback_data = {};
protected:
    int m_swap_interval = 1;
    GLFWwindow *w = nullptr;
    Parameters params = {};
    Camera camera = {};
    bool create_window(void);
public:
    NNGN_MOVE_ONLY(GLFWBackend)
    GLFWBackend(void) = default;
    explicit GLFWBackend(const Parameters &p) : params(p) {}
    ~GLFWBackend(void) override;
    bool init_glfw(void) const;
    bool window_closed(void) const final;
    int swap_interval(void) const final { return this->m_swap_interval; }
    nngn::uvec2 window_size(void) const override;
    void get_keys(size_t n, int32_t *keys) const override;
    ivec2 mouse_pos(void) const override;
    void set_swap_interval(int i) override;
    void set_window_title(const char *t) final;
    void set_cursor_mode(CursorMode m) final;
    void set_size_callback(void *data, size_callback_f f) override;
    void set_key_callback(void *data, key_callback_f f) override;
    void set_mouse_button_callback(
        void *data, mouse_button_callback_f f) override;
    void set_mouse_move_callback(
        void *data, mouse_move_callback_f f) override;
    void resize(int, int) override {}
    void set_camera(const Camera &c) override { this->camera = c; }
    void poll_events(void) const final;
    bool render(void) override;
};

}

#endif
