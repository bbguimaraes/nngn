/**
 * \dir
 * \brief Graphics back ends.
 *
 * Several graphics back ends are supported, substitutable at runtime:
 *
 * - \ref nngn::GLFWBackend "GLFWBackend": base for back ends that use GLFW.
 * - \ref nngn::Pseudograph "Pseudograph": headless back end for testing.
 *
 * The following areas of the screenshots page show some of the graphics
 * capabilities:
 *
 * - https://bbguimaraes.com/nngn/screenshots/engine.html
 */
#ifndef NNGN_GRAPHICS_GRAPHICS_H
#define NNGN_GRAPHICS_GRAPHICS_H

#include <memory>

#include "math/vec2.h"
#include "utils/def.h"
#include "utils/utils.h"

struct GLFWwindow;

namespace nngn {

struct Graphics {
    using key_callback_f = void (*)(void*, int, int, int, int);
    using mouse_button_callback_f = void (*)(void*, int, int, int);
    using mouse_move_callback_f = void (*)(void*, dvec2);
    enum class Backend : u8 {
        PSEUDOGRAPH, GLFW_BACKEND,
    };
    enum class CursorMode { NORMAL, HIDDEN, DISABLED };
    static std::unique_ptr<Graphics> create(Backend b, const void *params);
    NNGN_VIRTUAL(Graphics)
    // Initialization
    virtual bool init() = 0;
    // Operations
    virtual bool window_closed() const = 0;
    virtual int swap_interval() const = 0;
    virtual void set_swap_interval(int i) = 0;
    virtual void set_window_title(const char *t) = 0;
    virtual void set_cursor_mode(CursorMode m) = 0;
    virtual void set_key_callback(void *data, key_callback_f f) = 0;
    virtual void set_mouse_button_callback(
        void *data, mouse_button_callback_f f) = 0;
    virtual void set_mouse_move_callback(
        void *data, mouse_move_callback_f f) = 0;
    virtual void resize(int w, int h) = 0;
    // Rendering
    virtual void poll_events() const = 0;
    virtual bool render() = 0;
};

template<Graphics::Backend>
std::unique_ptr<Graphics> graphics_create_backend(const void *params);

}

#endif
