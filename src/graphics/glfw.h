#ifndef NNGN_GRAPHICS_GLFW_H
#define NNGN_GRAPHICS_GLFW_H

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
    Lighting lighting = {};
    bool create_window();
public:
    NNGN_NO_COPY(GLFWBackend)
    GLFWBackend() = default;
    explicit GLFWBackend(const Parameters &p) : params(p) {}
    ~GLFWBackend() override;
    bool init_glfw() const;
    bool window_closed() const final;
    int swap_interval() const final { return this->m_swap_interval; }
    nngn::uvec2 window_size() const override;
    void get_keys(size_t n, int32_t *keys) const override;
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
    void set_lighting(const Lighting &l) override { this->lighting = l; }
    void poll_events() const final;
    bool render() override;
};

}

#endif
