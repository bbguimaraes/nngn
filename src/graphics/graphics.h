/**
 * \dir
 * \brief Graphics back ends.
 *
 * Several graphics back ends are supported, substitutable at runtime:
 *
 * - \ref anonymous_namespace{vulkan.cpp}::VulkanBackend "VulkanBackend":
 *   main/default Vulkan renderer.
 * - \ref anonymous_namespace{opengl.cpp}::OpenGLBackend "OpenGLBackend": OpenGL
 *   ES / OpenGL 4 renderer.
 * - \ref nngn::Pseudograph "Pseudograph": headless back end for testing.
 *
 * The following areas of the screenshots page show some of the graphics
 * capabilities:
 *
 * - https://bbguimaraes.com/nngn/screenshots/engine.html
 */
#ifndef NNGN_GRAPHICS_GRAPHICS_H
#define NNGN_GRAPHICS_GRAPHICS_H

#include <cstdint>
#include <memory>
#include <string>
#include <tuple>

#include "math/vec2.h"
#include "utils/def.h"
#include "utils/flags.h"
#include "utils/utils.h"

struct GLFWwindow;

namespace nngn {

struct Graphics {
    using key_callback_f = void (*)(void*, int, int, int, int);
    using mouse_button_callback_f = void (*)(void*, int, int, int);
    using mouse_move_callback_f = void (*)(void*, dvec2);
    enum class Backend : u8 {
        PSEUDOGRAPH, OPENGL_BACKEND, OPENGL_ES_BACKEND, VULKAN_BACKEND,
    };
    enum class LogLevel { DEBUG, WARNING, ERROR };
    enum class PresentMode { IMMEDIATE, MAILBOX, FIFO, FIFO_RELAXED, };
    struct Parameters {
        enum Flag {
            HIDDEN = 1u << 0, DEBUG = 1u << 1,
        };
        Flags<Flag> flags = {};
    };
    struct Version { u32 major, minor, patch; const char *name; };
    struct OpenGLParameters : Parameters { int maj = {}, min = {}; };
    struct VulkanParameters : Parameters {
        Version version = {};
        LogLevel log_level = LogLevel::WARNING;
    };
    struct Extension {
        static constexpr std::size_t name_max_len = 256;
        std::array<char, name_max_len> name = {};
        u32 version = {};
    };
    struct Layer {
        std::array<char, 256> name = {};
        std::array<char, 256> description = {};
        std::array<char, 15> spec_version = {};
        u32 version = {};
    };
    struct DeviceInfo {
        enum class Type {
            OTHER, INTEGRATED_GPU, DISCRETE_GPU, VIRTUAL_GPU, CPU,
        };
        std::array<char, 256> name = {};
        std::array<char, 15> version = {};
        u32 driver_version = {}, vendor_id = {}, device_id = {};
        Type type = {};
    };
    struct QueueFamily {
        enum Flag : u8 {
            GRAPHICS = 1u << 0, COMPUTE = 1u << 1, TRANSFER = 1u << 2,
            PRESENT = 1u << 3,
        };
        u32 count = {};
        Flag flags = {};
    };
    struct MemoryHeap {
        enum Flag : u8 { DEVICE_LOCAL = 1u << 0 };
        u64 size = {};
        Flag flags = {};
    };
    struct MemoryType {
        enum Flag : std::uint8_t {
            DEVICE_LOCAL = 1u << 0, HOST_VISIBLE = 1u << 1,
            HOST_COHERENT = 1u << 2, HOST_CACHED = 1u << 3,
            LAZILY_ALLOCATED = 1u << 4,
        };
        Flag flags = {};
    };
    struct SurfaceInfo {
        u32 min_images = {}, max_images = {};
        uvec2 min_extent = {}, max_extent = {}, cur_extent = {};
    };
    struct PipelineConfiguration {
        enum class Type : u8 {
            TRIANGLE, MAX,
        };
        const char *name = {};
        Type type = {};
    };
    enum class CursorMode { NORMAL, HIDDEN, DISABLED };
    static std::unique_ptr<Graphics> create(Backend b, const void *params);
    static const char *enum_str(DeviceInfo::Type t);
    static const char *enum_str(QueueFamily::Flag f);
    static const char *enum_str(PresentMode m);
    static const char *enum_str(MemoryHeap::Flag m);
    static const char *enum_str(MemoryType::Flag m);
    static std::string flags_str(QueueFamily::Flag f);
    static std::string flags_str(MemoryHeap::Flag f);
    static std::string flags_str(MemoryType::Flag f);
    NNGN_VIRTUAL(Graphics)
    // Initialization
    /**
     * Fully initialize the back end.
     * Calls the other initialization functions with default values.
     */
    virtual bool init();
    virtual bool init_backend() = 0;
    virtual bool init_instance() = 0;
    virtual bool init_device() = 0;
    virtual bool init_device(std::size_t i) = 0;
    // Information
    virtual std::size_t n_extensions() const = 0;
    virtual std::size_t n_layers() const = 0;
    virtual std::size_t n_devices() const = 0;
    virtual std::size_t n_device_extensions(std::size_t i) const = 0;
    virtual std::size_t n_queue_families(std::size_t i) const = 0;
    virtual std::size_t n_present_modes() const = 0;
    virtual std::size_t n_heaps(std::size_t i) const = 0;
    virtual std::size_t n_memory_types(std::size_t i, std::size_t ih) const = 0;
    virtual std::size_t selected_device() const = 0;
    virtual void extensions(Extension *p) const = 0;
    virtual void layers(Layer *p) const = 0;
    virtual void device_infos(DeviceInfo *p) const = 0;
    virtual void device_extensions(std::size_t i, Extension *p) const = 0;
    virtual void queue_families(std::size_t i, QueueFamily *p) const = 0;
    virtual SurfaceInfo surface_info() const = 0;
    virtual void present_modes(PresentMode *p) const = 0;
    virtual void heaps(std::size_t i, MemoryHeap *p) const = 0;
    virtual void memory_types(
        std::size_t i, std::size_t ih, MemoryType *p) const = 0;
    // Operations
    virtual bool error() = 0;
    virtual Version version() const = 0;
    virtual bool window_closed() const = 0;
    virtual int swap_interval() const = 0;
    virtual bool set_n_frames(std::size_t n) = 0;
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
    virtual bool vsync() = 0;
};

inline bool Graphics::init() {
    return this->init_backend()
        && this->init_instance()
        && this->init_device({});
}

template<Graphics::Backend>
std::unique_ptr<Graphics> graphics_create_backend(const void *params);

}

#endif
