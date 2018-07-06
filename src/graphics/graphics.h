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

#include "utils/def.h"
#include "utils/utils.h"

struct GLFWwindow;

namespace nngn {

struct Graphics {
    enum class Backend : u8 {
        PSEUDOGRAPH, GLFW_BACKEND,
    };
    static std::unique_ptr<Graphics> create(Backend b, const void *params);
    NNGN_VIRTUAL(Graphics)
    // Initialization
    virtual bool init() = 0;
    // Operations
    virtual bool window_closed() const = 0;
    virtual int swap_interval() const = 0;
    virtual void set_swap_interval(int i) = 0;
    virtual void set_window_title(const char *t) = 0;
    virtual void resize(int w, int h) = 0;
    // Rendering
    virtual void poll_events() const = 0;
    virtual bool render() = 0;
};

template<Graphics::Backend>
std::unique_ptr<Graphics> graphics_create_backend(const void *params);

}

#endif
