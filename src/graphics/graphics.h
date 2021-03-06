#ifndef NNGN_GRAPHICS_GRAPHICS_H
#define NNGN_GRAPHICS_GRAPHICS_H

#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <tuple>

#include "const.h"

#include "math/mat4.h"
#include "math/math.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include "utils/def.h"
#include "utils/flags.h"
#include "utils/utils.h"

#include "stats.h"

struct GLFWwindow;

namespace nngn {

struct CameraUBO { mat4 proj, view; };
struct LightsUBO {
    enum Flag : uint32_t {
        SHADOWS_ENABLED = NNGN_SHADOWS_ENABLED_BIT,
    };
    uint32_t flags = 0;
    float depth_transform0 = {}, depth_transform1 = {};
    alignas(16) vec3 view_pos = {};
    uint32_t n_dir = 0;
    vec3 ambient = {};
    uint32_t n_point = 0;
    struct {
        std::array<vec4, NNGN_MAX_LIGHTS> dir = {};
        std::array<vec4, NNGN_MAX_LIGHTS> color_spec = {};
        std::array<mat4, NNGN_MAX_LIGHTS> mat = {};
    } dir;
    struct {
        std::array<vec4, NNGN_MAX_LIGHTS> dir = {};
        std::array<vec4, NNGN_MAX_LIGHTS> color_spec = {};
        std::array<vec4, NNGN_MAX_LIGHTS> pos = {};
        std::array<vec4, NNGN_MAX_LIGHTS> att_cutoff = {};
    } point;
};
struct Vertex { vec3 pos, norm, color; };

struct Graphics {
    using size_callback_f = void (*)(void*, uvec2);
    using key_callback_f = void (*)(void*, int, int, int, int);
    using mouse_button_callback_f = void (*)(void*, int, int, int);
    using mouse_move_callback_f = void (*)(void*, dvec2);
    enum class Backend : u8 {
        PSEUDOGRAPH, TERMINAL_BACKEND,
        OPENGL_BACKEND, OPENGL_ES_BACKEND, VULKAN_BACKEND,
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
    enum class TerminalMode { ASCII, COLORED };
    struct TerminalParameters {
        int fd = -1;
        TerminalMode mode = TerminalMode::ASCII;
    };
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
        enum Flag : u8 {
            DEPTH_TEST = 1u << 0,
            DEPTH_WRITE = 1u << 1,
            CULL_BACK_FACES = 1u << 2,
            LINE = 1u << 3,
        };
        enum class Type : u8 {
            TRIANGLE, SPRITE, VOXEL, FONT, TRIANGLE_DEPTH, SPRITE_DEPTH, MAX,
        };
        const char *name = {};
        Type type = {};
        Flag flags = {};
    };
    struct BufferConfiguration {
        enum class Type { VERTEX, INDEX };
        const char *name = {};
        Type type = {};
        u64 size = {};
    };
    struct RenderList {
        struct Stage {
            u32 pipeline = {};
            std::span<const std::pair<u32, u32>> buffers = {};
        };
        std::span<const Stage>
            depth = {}, map_ortho = {}, map_persp = {},
            normal = {}, no_light = {}, overlay = {}, hud = {},
            shadow_maps = {}, shadow_cubes = {};
    };
    enum class CursorMode { NORMAL, HIDDEN, DISABLED };
    struct Camera {
        uint8_t *flags = nullptr;
        const uvec2 *screen = nullptr;
        const mat4 *proj = nullptr, *hud_proj = nullptr, *view = nullptr;
    };
    struct Lighting {
        const LightsUBO *ubo = nullptr;
        const mat4
            *dir_proj = nullptr, *point_proj = nullptr,
            *dir_views = nullptr, *point_views = nullptr;
    };
    static constexpr u32
        TEXTURE_EXTENT = 512,
        TEXTURE_EXTENT_LOG2 = std::countr_zero(std::bit_floor(TEXTURE_EXTENT)),
        TEXTURE_SIZE = 4 * TEXTURE_EXTENT * TEXTURE_EXTENT,
        TEXTURE_MIP_LEVELS = Math::mip_levels(TEXTURE_EXTENT),
        SHADOW_MAP_INITIAL_SIZE = 1024,
        SHADOW_CUBE_INITIAL_SIZE = 512;
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
    virtual uvec2 window_size() const = 0;
    virtual GraphicsStats stats() = 0;
    virtual void get_keys(size_t n, int32_t *keys) const = 0;
    virtual bool set_n_frames(std::size_t n) = 0;
    virtual void set_swap_interval(int i) = 0;
    virtual void set_window_title(const char *t) = 0;
    virtual void set_cursor_mode(CursorMode m) = 0;
    virtual void set_size_callback(void *data, size_callback_f f) = 0;
    virtual void set_key_callback(void *data, key_callback_f f) = 0;
    virtual void set_mouse_button_callback(
        void *data, mouse_button_callback_f f) = 0;
    virtual void set_mouse_move_callback(
        void *data, mouse_move_callback_f f) = 0;
    virtual void resize(int w, int h) = 0;
    virtual void set_camera(const Camera &c) = 0;
    virtual void set_lighting(const Lighting &l) = 0;
    virtual void set_camera_updated() = 0;
    virtual void set_lighting_updated() = 0;
    virtual bool set_shadow_map_size(uint32_t s) = 0;
    virtual bool set_shadow_cube_size(uint32_t s) = 0;
    // Pipelines
    virtual u32 create_pipeline(const PipelineConfiguration &conf) = 0;
    // Buffers
    virtual u32 create_buffer(const BufferConfiguration &conf) = 0;
    virtual bool set_buffer_capacity(u32 b, u64 size) = 0;
    virtual void set_buffer_size(u32 b, std::uint64_t size) = 0;
    virtual bool write_to_buffer(
        u32 b, std::uint64_t offset,
        std::uint64_t n, std::uint64_t size,
        void *data, void f(void*, void*, std::uint64_t, std::uint64_t)) = 0;
    bool update_buffers(
        u32 vbo, u32 ebo, u64 voff, u64 eoff,
        u64 vn, u64 vsize, u64 en, u64 esize,
        void *data, auto &&vgen, auto &&egen);
    // Textures
    virtual bool resize_textures(std::uint32_t s) = 0;
    virtual bool load_textures(
        std::uint32_t i, std::uint32_t n, const std::byte *v) = 0;
    // Fonts
    virtual bool resize_font(std::uint32_t s) = 0;
    virtual bool load_font(
        unsigned char c, std::uint32_t n,
        const nngn::uvec2 *size, const std::byte *v) = 0;
    // Rendering
    virtual bool set_render_list(const RenderList &l) = 0;
    virtual void poll_events() const = 0;
    virtual bool render() = 0;
    virtual bool vsync() = 0;
};

inline bool Graphics::init() {
    return this->init_backend()
        && this->init_instance()
        && this->init_device({});
}

inline bool Graphics::update_buffers(
    u32 vbo, u32 ebo, u64 voff, u64 eoff,
    u64 vn, u64 vsize, u64 en, u64 esize,
    void *data, auto &&vgen, auto &&egen
) {
    const bool ok = this->write_to_buffer(vbo, voff, vn, vsize, data, vgen)
        && this->write_to_buffer(ebo, eoff, en, esize, data, egen);
    if(!ok)
        return false;
    this->set_buffer_size(vbo, voff + vn * vsize);
    this->set_buffer_size(ebo, eoff + en * esize);
    return true;
}

template<Graphics::Backend>
std::unique_ptr<Graphics> graphics_create_backend(const void *params);

}

#endif
