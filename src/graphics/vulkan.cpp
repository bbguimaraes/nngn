#include "os/platform.h"
#include "utils/log.h"

#include "graphics.h"

#ifndef NNGN_PLATFORM_HAS_VULKAN

namespace nngn {

template<> std::unique_ptr<Graphics> graphics_create_backend
        <Graphics::Backend::VULKAN_BACKEND>(const void*) {
    NNGN_LOG_CONTEXT_F();
    nngn::Log::l() << "compiled without Vulkan support\n";
    return nullptr;
}

}

#else

#include <algorithm>
#include <cassert>
#include <cstring>
#include <iomanip>
#include <list>
#include <vector>
using namespace std::literals;

#include "font/font.h"
#include "math/camera.h"
#include "math/math.h"
#include "render/light.h"
#include "timing/profile.h"
#include "utils/flags.h"
#include "utils/scoped.h"
#include "utils/utils.h"

#include "glfw.h"
#include "shaders.h"

namespace {

struct Handle {
    std::uint64_t id;
    Handle(std::uint64_t id_ = {}) : id(id_) {}
    operator std::uint64_t() const { return this->id; }
};

}

#if !(defined(_WIN64) || defined(__x86_64__))
#define VK_DEFINE_NON_DISPATCHABLE_HANDLE(o) \
    struct o : Handle { using Handle::Handle; };
#endif

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

namespace nngn {

static constexpr VkFormat SURFACE_FORMAT = VK_FORMAT_B8G8R8A8_UNORM;
static constexpr VkFormat DEPTH_FORMAT = VK_FORMAT_D32_SFLOAT;
static constexpr auto SWAPCHAIN_EXTENSION = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
static constexpr std::array VALIDATION_LAYERS = {
    "VK_LAYER_KHRONOS_validation"};
const auto CLIP_PROJ = Math::transpose(mat4(
    1,  0,  0,  0,
    0,  1,  0,  0,
    0,  0, .5, .5,
    0,  0,  0,  1));

class Instance {
    PFN_vkSetDebugUtilsObjectNameEXT set_obj_name_fp = nullptr;
    VkDebugUtilsMessengerEXT messenger = {};
    bool create_messenger(Graphics::LogLevel log_level);
    template<typename F> F get_proc_addr(const char *name) const;
    template<typename F, typename ...A>
        bool call(const char *name, A &&...args) const;
    template<typename F, typename ...A>
        bool check_call(const char *name, A &&...args) const;
public:
    VkInstance id = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    bool error = false;
    bool create(GLFWwindow *w, Graphics::LogLevel log_level);
    bool create_debug(GLFWwindow *w, Graphics::LogLevel log_level);
    void destroy();
    bool set_obj_name(
        VkDevice dev, VkObjectType type,
        uint64_t obj, std::string_view name) const;
    bool set_obj_name_n(
        VkDevice dev, VkObjectType type,
        uint64_t obj, std::string_view name, size_t n) const;
    template<typename T> bool set_obj_name(
        VkDevice dev, T obj, std::string_view name) const;
    template<typename T> bool set_obj_name_n(
        VkDevice dev, T obj, std::string_view name, size_t n) const;
};

template<typename T> static constexpr VkObjectType obj_type = {};
template<> auto obj_type<VkBuffer> = VK_OBJECT_TYPE_BUFFER;
template<> auto obj_type<VkCommandBuffer> = VK_OBJECT_TYPE_COMMAND_BUFFER;
template<> auto obj_type<VkCommandPool> = VK_OBJECT_TYPE_COMMAND_POOL;
template<> auto obj_type<VkDescriptorPool> = VK_OBJECT_TYPE_DESCRIPTOR_POOL;
template<> auto obj_type<VkDescriptorSet> = VK_OBJECT_TYPE_DESCRIPTOR_SET;
template<> auto obj_type<VkDescriptorSetLayout> =
    VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT;
template<> auto obj_type<VkDeviceMemory> = VK_OBJECT_TYPE_DEVICE_MEMORY;
template<> auto obj_type<VkFence> = VK_OBJECT_TYPE_FENCE;
template<> auto obj_type<VkFramebuffer> = VK_OBJECT_TYPE_FRAMEBUFFER;
template<> auto obj_type<VkImage> = VK_OBJECT_TYPE_IMAGE;
template<> auto obj_type<VkImageView> = VK_OBJECT_TYPE_IMAGE_VIEW;
template<> auto obj_type<VkPipeline> = VK_OBJECT_TYPE_PIPELINE;
template<> auto obj_type<VkPipelineLayout> = VK_OBJECT_TYPE_PIPELINE_LAYOUT;
template<> auto obj_type<VkRenderPass> = VK_OBJECT_TYPE_RENDER_PASS;
template<> auto obj_type<VkSampler> = VK_OBJECT_TYPE_SAMPLER;
template<> auto obj_type<VkSemaphore> = VK_OBJECT_TYPE_SEMAPHORE;
template<> auto obj_type<VkShaderModule> = VK_OBJECT_TYPE_SHADER_MODULE;

struct Device;

struct PhysicalDevice {
    VkPhysicalDevice id = VK_NULL_HANDLE;
    VkSurfaceCapabilitiesKHR surface_cap = {};
    bool create_device(
        const Instance &inst, uint32_t queue_family, Device *dev) const;
    void update_surface_cap(VkSurfaceKHR surface);
};

struct DeviceMemory {
    VkPhysicalDeviceMemoryProperties props = {};
    std::vector<VkDeviceMemory> allocs = {};
    void init(VkPhysicalDevice physical_dev);
    void destroy();
    bool find_type(
        uint32_t type, VkMemoryPropertyFlags f, uint32_t *ret) const;
    bool alloc(
        VkDevice dev, VkMemoryRequirements req, VkMemoryPropertyFlags f,
        VkDeviceMemory *ret);
    bool alloc(
        VkDevice dev, VkBuffer buf, VkMemoryPropertyFlags f,
        VkDeviceMemory *ret);
    bool alloc(
        VkDevice dev, VkImage img, VkMemoryPropertyFlags f,
        VkDeviceMemory *ret);
    void dealloc(VkDevice dev, VkDeviceMemory mem);
};

struct CommandPool {
    VkCommandPool id = VK_NULL_HANDLE;
    bool init(
        VkDevice dev, VkCommandPoolCreateFlags flags, uint32_t queue_family);
    void destroy(VkDevice dev);
    template<typename F> bool with_cmd_buffer(
        VkDevice dev, VkQueue queue, F f) const;
};

struct DescriptorPool {
    VkDescriptorPool id = VK_NULL_HANDLE;
    bool init(
        VkDevice dev, uint32_t max,
        uint32_t n_sizes, const VkDescriptorPoolSize *sizes);
    void destroy(VkDevice dev);
};

struct DescriptorSets {
    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    DescriptorPool pool = {};
    std::vector<VkDescriptorSet> ids = {};
    bool init(
        VkDevice dev, uint32_t max,
        uint32_t n_bindings, const VkDescriptorSetLayoutBinding *bindings);
    void destroy(VkDevice dev);
};

struct Buffer;

struct CameraDescriptorSets : public DescriptorSets {
    bool init(VkDevice dev, uint32_t max);
    bool write(
        VkDevice dev, uint32_t max,
        const Buffer *ubos, const Buffer *hud_ubos,
        const Buffer *shadow_ubos) const;
};

struct LightingDescriptorSets : public DescriptorSets {
    bool init(VkDevice dev, uint32_t max);
    bool write(
        VkDevice dev, uint32_t max, const Buffer *ubos,
        VkSampler sampler,
        VkImageView shadow_map_view, VkImageView shadow_cube_view) const;
};

struct TextureDescriptorSets : public DescriptorSets {
    bool init(VkDevice dev);
    bool write(
        VkDevice dev, VkSampler tex_sampler, VkSampler shadow_sampler,
        VkImageView tex_img_view, VkImageView font_img_view,
        VkImageView shadow_map_img_view,
        VkImageView shadow_cube_img_view) const;
};

struct Image {
    VkImage id = VK_NULL_HANDLE;
    VkDeviceMemory mem = VK_NULL_HANDLE;
    void destroy(DeviceMemory *dev_mem, VkDevice dev);
    bool transition_layout(
        VkDevice dev, VkQueue queue, const CommandPool &cmd_pool,
        VkImageAspectFlags aspect, VkImageLayout src, VkImageLayout dst,
        uint32_t mip_levels, uint32_t base_layer, uint32_t n_layers) const;
};

struct TexArray;

class Swapchain {
    bool create_img_views(const Instance &inst, Device *dev);
    bool create_depth_img(
        const Instance &inst, DeviceMemory *dev_mem, const Device &dev,
        const VkExtent2D &extent);
    bool create_render_pass(const Instance &inst, VkDevice dev);
    bool create_depth_pass(
        const Instance &inst, VkDevice dev,
        VkExtent2D shadow_map_size, VkExtent2D shadow_cube_size,
        const TexArray &shadow_map, const TexArray &shadow_cube);
    bool create_descriptor_set_layout(
        const Instance &inst, VkDevice dev);
    bool create_graphics_pipelines(
        const Instance &inst, Device *dev, const VkExtent2D &extent,
        VkExtent2D shadow_map_size, VkExtent2D shadow_cube_size);
    bool create_framebuffers(
        const Instance &inst, VkDevice dev, const VkExtent2D &extent);
    bool create_sync_objects(const Instance &inst, VkDevice dev);
public:
    VkSwapchainKHR id = VK_NULL_HANDLE;
    std::vector<VkImageView> img_views;
    Image depth_img = {};
    VkImageView depth_img_view = VK_NULL_HANDLE;
    VkRenderPass render_pass = VK_NULL_HANDLE, depth_pass = VK_NULL_HANDLE;
    CameraDescriptorSets camera_descriptor_sets;
    LightingDescriptorSets lighting_descriptor_sets;
    TextureDescriptorSets texture_descriptor_sets;
    VkPipelineLayout render_pipeline_layout = VK_NULL_HANDLE;
    VkPipelineLayout depth_triangle_pipeline_layout = VK_NULL_HANDLE;
    VkPipelineLayout depth_sprite_pipeline_layout = VK_NULL_HANDLE;
    VkPipeline triangle_pipeline = VK_NULL_HANDLE;
    VkPipeline sprite_pipeline = VK_NULL_HANDLE;
    std::array<VkPipeline, 2>
        triangle_depth_pipeline = {{VK_NULL_HANDLE, VK_NULL_HANDLE}},
        sprite_depth_pipeline = {{VK_NULL_HANDLE, VK_NULL_HANDLE}};
    VkPipeline box_pipeline = VK_NULL_HANDLE;
    VkPipeline font_pipeline = VK_NULL_HANDLE;
    VkPipeline grid_pipeline = VK_NULL_HANDLE;
    VkPipeline circle_pipeline = VK_NULL_HANDLE;
    VkPipeline wire_pipeline = VK_NULL_HANDLE;
    VkPipeline map_ortho_pipeline = VK_NULL_HANDLE;
    VkPipeline map_persp_pipeline = VK_NULL_HANDLE;
    std::array<VkPipeline, 2> map_depth_pipeline =
        {VK_NULL_HANDLE, VK_NULL_HANDLE};
    std::vector<VkFramebuffer> framebuffers, depth_framebuffers;
    std::vector<VkSemaphore> image_available_sem, render_finished_sem;
    std::vector<VkFence> fences;
    bool init(
        const Instance &inst, DeviceMemory *dev_mem, Device *dev,
        const VkExtent2D &extent,
        VkExtent2D shadow_map_size, VkExtent2D shadow_cube_size,
        const TexArray &shadow_map, const TexArray &shadow_cube);
    bool recreate(
        const Instance &inst, DeviceMemory *dev_mem, Device *dev,
        const VkExtent2D &extent,
        VkExtent2D shadow_map_size, VkExtent2D shadow_cube_size,
        const TexArray &shadow_map, const TexArray &shadow_cube);
    void soft_destroy(DeviceMemory *dev_mem, VkDevice dev);
    void destroy(DeviceMemory *dev_mem, VkDevice dev);
};

struct Device {
    VkDevice id = VK_NULL_HANDLE;
    VkQueue graphics_queue = VK_NULL_HANDLE, present_queue = VK_NULL_HANDLE;
    Swapchain swapchain = {};
    CommandPool cmd_pool = {}, tmp_cmd_pool = {};
    bool init(const Instance &inst, uint32_t queue_family);
    bool create_img(
        DeviceMemory *dev_mem,
        VkExtent2D extent, uint32_t mip_levels, uint32_t n_layers,
        VkFormat fmt, VkImageTiling tiling, VkImageUsageFlags usage,
        VkSampleCountFlagBits n_samples, VkImageCreateFlags flags,
        VkMemoryPropertyFlags mem_props, Image *img) const;
    bool create_img_view(
        VkImage img, VkFormat fmt, VkImageViewType type,
        VkImageAspectFlags aspect_flags,
        uint32_t mip_levels, uint32_t base_layer, uint32_t n_layers,
        VkImageView *img_view) const;
    bool create_sampler(
        VkFilter filter, VkSamplerAddressMode addr_mode, VkBorderColor border,
        VkSamplerMipmapMode mip_mode, uint32_t mip_levels,
        VkSampler *sampler) const;
    bool create_shader(
        const Instance &inst, std::string_view name, std::string_view src,
        VkShaderModule *m) const;
    void destroy(DeviceMemory *dev_mem);
};

struct Buffer {
    VkBuffer id = VK_NULL_HANDLE;
    VkDeviceMemory mem = VK_NULL_HANDLE;
    uint64_t size = 0, capacity = 0;
    bool init(
        VkDevice dev, DeviceMemory *dev_mem,
        VkBufferUsageFlags usage, VkMemoryPropertyFlags memp,
        VkDeviceSize size);
    void destroy(DeviceMemory *dev_mem, VkDevice dev);
    void memcpy(VkDevice dev, const void *p, size_t size) const;
};

struct TexArray {
    uint32_t mip_levels = 0;
    Image img = {};
    VkImageView img_view = VK_NULL_HANDLE, cube_2d_img_view = VK_NULL_HANDLE;
    std::vector<VkImageView> layer_views = {};
    bool init(
        Device *dev, DeviceMemory *dev_mem,
        VkQueue queue, const CommandPool &cmd_pool,
        VkExtent2D extent, uint32_t mip_levels, uint32_t n,
        VkFormat fmt, VkImageUsageFlags usage, VkImageCreateFlags flags,
        VkImageViewType view_type, VkImageAspectFlags view_aspects,
        VkImageLayout layout);
    bool init_mipmaps(
        VkDevice dev, VkQueue queue, const CommandPool &cmd_pool,
        VkExtent2D extent, uint32_t base_layer, uint32_t n_layers);
    bool init_layer_views(
        Device *dev, uint32_t n, VkFormat fmt,
        VkImageViewType view_type, VkImageAspectFlags view_aspects);
    void destroy(DeviceMemory *dev_mem, VkDevice dev);
};

struct ShadowMap : public TexArray {
    bool init(
        Device *dev, DeviceMemory *dev_mem,
        VkQueue queue, const CommandPool &cmd_pool, VkExtent2D extent);
};

struct ShadowCube : public TexArray {
    bool init(
        Device *dev, DeviceMemory *dev_mem,
        VkQueue queue, const CommandPool &cmd_pool, VkExtent2D extent);
};

class VulkanBackend final : public GLFWBackend {
    static constexpr uint32_t graphics_family = 0, present_family = 0;
    enum Flag : uint8_t {
        RECREATE_SWAPCHAIN = 1u << 0, RECREATE_CMDS = 1u << 1,
    };
    Flags<Flag> flags = {Flag::RECREATE_CMDS};
    LogLevel log_level;
    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    size_t iframe = 0;
    uint8_t camera_updated = 0, lighting_updated = 0;
    uint32_t shadow_map_size = SHADOW_MAP_INITIAL_SIZE;
    uint32_t shadow_cube_size = SHADOW_CUBE_INITIAL_SIZE;
    Instance instance = {};
    PhysicalDevice physical_dev = {};
    Device dev = {};
    DeviceMemory dev_mem = {};
    std::vector<VkCommandBuffer> cmd_buffers = {};
    std::vector<Buffer> camera_ubos = {}, camera_hud_ubos = {};
    std::vector<Buffer> camera_shadow_ubos = {}, lights_ubos = {};
    std::list<Buffer> stg_buffers = {};
    Buffer triangle_vbo = {}, triangle_ebo = {};
    Buffer sprite_vbo = {}, sprite_ebo = {};
    Buffer translucent_vbo = {}, translucent_ebo = {};
    Buffer box_vbo = {}, box_ebo = {};
    Buffer cube_vbo = {}, cube_ebo = {}, cube_dbg_vbo = {}, cube_dbg_ebo = {};
    Buffer sphere_vbo = {}, sphere_ebo = {};
    Buffer sphere_dbg_vbo = {}, sphere_dbg_ebo = {};
    Buffer text_vbo = {}, text_ebo = {};
    Buffer textbox_vbo = {}, textbox_ebo = {};
    Buffer sel_vbo = {}, sel_ebo = {}, grid_vbo = {}, grid_ebo = {};
    Buffer aabb_vbo = {}, aabb_ebo = {};
    Buffer aabb_circle_vbo = {}, aabb_circle_ebo = {};
    Buffer bb_vbo = {}, bb_ebo = {}, bb_circle_vbo = {}, bb_circle_ebo = {};
    Buffer sphere_coll_vbo = {}, sphere_coll_ebo = {};
    Buffer lights_vbo = {}, lights_ebo = {}, range_vbo = {}, range_ebo = {};
    Buffer depth_vbo = {}, depth_ebo = {};
    Buffer depth_cube_vbo = {}, depth_cube_ebo = {};
    Buffer map_vbo = {}, map_ebo = {};
    VkSampler tex_sampler = {}, shadow_sampler = {};
    TexArray tex = {}, font_tex = {};
    ShadowMap shadow_map = {};
    ShadowCube shadow_cube = {};
    void resize(int, int) final;
    bool find_device();
    bool create_swapchain();
    bool recreate_swapchain();
    bool name_tex_array(std::string_view name, const TexArray &t) const;
    bool create_uniform_buffer(
        std::string_view name,
        size_t n, VkDeviceSize size, std::vector<Buffer> *ubos);
    bool create_vertex_buffer(
        std::string_view name, VkDeviceSize size, Buffer *b);
    bool create_index_buffer(
        std::string_view name, VkDeviceSize size, Buffer *b);
    bool copy_buffer(
        VkBuffer dst, VkBuffer src, const VkBufferCopy &copy) const;
    bool copy_buffer(
        VkExtent2D extent, uint32_t base_layer, uint32_t n_layers,
        VkBuffer src, VkImage dst) const;
    bool create_cmd_buffers();
    Buffer *buffer_target(TargetBuffer b);
    bool copy_to_buffer(
        VkBuffer b, const StagingBuffer &stg,
        uint64_t offset, uint64_t length);
public:
    NNGN_NO_COPY(VulkanBackend)
    explicit VulkanBackend(const VulkanParameters &p);
    ~VulkanBackend() final;
    std::tuple<int, int, int, std::string> version() const final;
    bool init() final;
    bool error() override { return this->instance.error; }
    void set_swap_interval(int i) final;
    void set_camera_updated() final
        { this->camera_updated = std::numeric_limits<uint8_t>::max(); }
    void set_lighting_updated() final
        { this->lighting_updated = std::numeric_limits<uint8_t>::max(); }
    bool set_shadow_map_size(uint32_t s) final;
    bool set_shadow_cube_size(uint32_t s) final;
    bool create_stg_buffer(uint64_t size, StagingBuffer *stg) final;
    bool create_stg_buffer_pair(
        uint64_t vsize, uint64_t isize,
        std::pair<StagingBuffer, StagingBuffer> *stg) final;
    bool unmap_stg_buffer(StagingBuffer *stg) final;
    bool copy_to_buffer(
        TargetBuffer b, const StagingBuffer &stg,
        uint64_t offset, uint64_t length) final;
    bool destroy_stg_buffer(StagingBuffer *stg) final;
    bool set_buffer_pair_capacity(
        TargetBuffer vbo, uint64_t vcap,
        TargetBuffer ebo, uint64_t ecap) final;
    bool set_buffer_size(TargetBuffer, uint64_t size) final;
    bool resize_textures(uint32_t s) final;
    bool load_textures(uint32_t i, uint32_t n, const std::byte *v) final;
    bool resize_font(uint32_t s) final;
    bool load_font(
        unsigned char c, uint32_t n,
        const nngn::uvec2 *size, const std::byte *v) final;
    bool render() final;
};

template<> std::unique_ptr<Graphics> graphics_create_backend
        <Graphics::Backend::VULKAN_BACKEND>(const void *params) {
    return std::make_unique<VulkanBackend>(
        params
            ? *static_cast<const Graphics::VulkanParameters*>(params)
            : Graphics::VulkanParameters{});
}

static const char *vk_strerror(VkResult result) {
    switch(result) {
#define C(x) case x: return #x;
    C(VK_SUCCESS)
    C(VK_NOT_READY)
    C(VK_TIMEOUT)
    C(VK_EVENT_SET)
    C(VK_EVENT_RESET)
    C(VK_INCOMPLETE)
    C(VK_ERROR_OUT_OF_HOST_MEMORY)
    C(VK_ERROR_OUT_OF_DEVICE_MEMORY)
    C(VK_ERROR_INITIALIZATION_FAILED)
    C(VK_ERROR_DEVICE_LOST)
    C(VK_ERROR_MEMORY_MAP_FAILED)
    C(VK_ERROR_LAYER_NOT_PRESENT)
    C(VK_ERROR_EXTENSION_NOT_PRESENT)
    C(VK_ERROR_FEATURE_NOT_PRESENT)
    C(VK_ERROR_INCOMPATIBLE_DRIVER)
    C(VK_ERROR_TOO_MANY_OBJECTS)
    C(VK_ERROR_FORMAT_NOT_SUPPORTED)
    C(VK_ERROR_FRAGMENTED_POOL)
#ifdef VK_VERSION_1_2
    C(VK_ERROR_UNKNOWN)
#endif
#ifdef VK_VERSION_1_1
    C(VK_ERROR_OUT_OF_POOL_MEMORY)
    C(VK_ERROR_INVALID_EXTERNAL_HANDLE)
#endif
#ifdef VK_EXT_descriptor_indexing
    C(VK_ERROR_FRAGMENTATION_EXT)
#endif
#ifdef VK_VERSION_1_2
    C(VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS)
#endif
#ifdef VK_KHR_surface
    C(VK_ERROR_SURFACE_LOST_KHR)
    C(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR)
#endif
#ifdef VK_KHR_swapchain
    C(VK_SUBOPTIMAL_KHR)
    C(VK_ERROR_OUT_OF_DATE_KHR)
#endif
#ifdef VK_KHR_display_swapchain
    C(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR)
#endif
#ifdef VK_EXT_debug_report
    C(VK_ERROR_VALIDATION_FAILED_EXT)
#endif
#ifdef VK_NV_glsl_shader
    C(VK_ERROR_INVALID_SHADER_NV)
#endif
#if 135 <= VK_HEADER_VERSION && VK_HEADER_VERSION <= 161
    C(VK_ERROR_INCOMPATIBLE_VERSION_KHR)
#endif
#ifdef VK_EXT_image_drm_format_modifier
    C(VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT)
#endif
#ifdef VK_EXT_global_priority
    C(VK_ERROR_NOT_PERMITTED_EXT)
#endif
#if VK_HEADER_VERSION >= 105
    C(VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT)
#endif
#if VK_HEADER_VERSION >= 135
    C(VK_THREAD_IDLE_KHR)
    C(VK_THREAD_DONE_KHR)
    C(VK_OPERATION_DEFERRED_KHR)
    C(VK_OPERATION_NOT_DEFERRED_KHR)
#if defined(VK_EXT_buffer_device_address) && !defined(VK_VERSION_1_2)
    C(VK_ERROR_INVALID_DEVICE_ADDRESS_EXT)
#endif
    C(VK_ERROR_PIPELINE_COMPILE_REQUIRED_EXT)
#endif
#if !defined(VK_VERSION_1_2) || VK_HEADER_VERSION < 140
    C(VK_RESULT_RANGE_SIZE)
#endif
    C(VK_RESULT_MAX_ENUM)
    default: return "unknown";
    }
}

static const char *vk_enum_str(VkDebugUtilsMessageSeverityFlagBitsEXT f) {
    switch(f) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: return "verbose";
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: return "info";
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: return "warning";
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: return "error";
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
    default: return "unknown";
    }
}

static const char *vk_enum_str(VkDebugUtilsMessageTypeFlagsEXT f) {
    switch(f) {
    case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: return "general";
    case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: return "validation";
    case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: return "performance";
    case VK_DEBUG_UTILS_MESSAGE_TYPE_FLAG_BITS_MAX_ENUM_EXT:
    default: return "unknown";
    }
}

static bool check_result(const char *func_name, VkResult result) {
    if(result == VK_SUCCESS)
        return true;
    Log::l() << func_name << ": " << vk_strerror(result) << std::endl;
    return false;
}
#define CHECK_RESULT(f, ...) \
    if(!check_result(#f, f(__VA_ARGS__))) return false;


template<typename T, typename F, typename... P>
static auto enumerate(F f, const P &...args) {
    uint32_t n = 0;
    f(args..., &n, nullptr);
    std::vector<T> ret(n);
    f(args..., &n, ret.data());
    return ret;
}

template<typename T, typename F, typename ...A>
static void destroy(T *t, F f, A ...a)
    { if(*t) f(a..., std::exchange(*t, {}), nullptr); }

bool Instance::create(GLFWwindow *w, Graphics::LogLevel) {
    NNGN_LOG_CONTEXT_CF(Instance);
    VkInstanceCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    info.enabledLayerCount = 0;
    info.ppEnabledExtensionNames =
        glfwGetRequiredInstanceExtensions(
            &info.enabledExtensionCount);
    CHECK_RESULT(vkCreateInstance, &info, nullptr, &this->id);
    CHECK_RESULT(
        glfwCreateWindowSurface, this->id, w, nullptr, &this->surface);
    return true;
}

bool Instance::create_debug(GLFWwindow *w, Graphics::LogLevel log_level) {
    NNGN_LOG_CONTEXT_CF(Instance);
    VkInstanceCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    info.enabledLayerCount = VALIDATION_LAYERS.size();
    info.ppEnabledLayerNames = VALIDATION_LAYERS.data();
    uint32_t n = {};
    const auto *glfw_ext = glfwGetRequiredInstanceExtensions(&n);
    std::vector<const char*> ext;
    ext.reserve(n + 1);
    std::copy(glfw_ext, glfw_ext + n, std::back_inserter(ext));
    ext.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    info.enabledExtensionCount = static_cast<uint32_t>(ext.size());
    info.ppEnabledExtensionNames = ext.data();
    CHECK_RESULT(vkCreateInstance, &info, nullptr, &this->id);
    if(!this->create_messenger(log_level))
        return false;
    if(!(this->set_obj_name_fp =
            this->get_proc_addr<PFN_vkSetDebugUtilsObjectNameEXT>(
                "vkSetDebugUtilsObjectNameEXT")))
        return false;
    CHECK_RESULT(
        glfwCreateWindowSurface, this->id, w, nullptr, &this->surface);
    return true;
}

bool Instance::create_messenger(Graphics::LogLevel log_level) {
    NNGN_LOG_CONTEXT_CF(Instance);
    VkDebugUtilsMessengerCreateInfoEXT info = {};
    info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    switch(log_level) {
    case Graphics::LogLevel::DEBUG:
        info.messageSeverity |=
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
        [[fallthrough]];
    case Graphics::LogLevel::WARNING:
        info.messageSeverity |=
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
        [[fallthrough]];
    case Graphics::LogLevel::ERROR: break;
    }
    info.messageSeverity |=
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    info.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    info.pUserData = this;
    info.pfnUserCallback = [](
            VkDebugUtilsMessageSeverityFlagBitsEXT severity,
            VkDebugUtilsMessageTypeFlagsEXT type,
            const auto *data, void *p) {
        Log::l()
            << "vulkan_debug_callback("
            << vk_enum_str(severity) << ", " << vk_enum_str(type)
            << "): " << data->pMessage << '\n';
        if(severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
            static_cast<Instance*>(p)->error = true;
        return static_cast<VkBool32>(VK_FALSE);
    };
    return this->check_call<PFN_vkCreateDebugUtilsMessengerEXT>(
        "vkCreateDebugUtilsMessengerEXT",
        this->id, &info, nullptr, &this->messenger);
}

template<typename F>
F Instance::get_proc_addr(const char *name) const {
    const auto ret =  reinterpret_cast<F>(
        vkGetInstanceProcAddr(this->id, name));
    if(!ret)
        Log::l()
            << "Instance::get_proc_addr: "
            << name << " not present" << std::endl;
    return ret;
}

template<typename F, typename ...A>
bool Instance::call(const char *name, A &&...args) const {
    auto fp = this->get_proc_addr<F>(name);
    return fp ? (fp(args...), true) : false;
}

template<typename F, typename ...A>
bool Instance::check_call(const char *name, A &&...args) const {
    auto fp = this->get_proc_addr<F>(name);
    return fp ? (fp(args...) == VK_SUCCESS) : false;
}

void Instance::destroy() {
    NNGN_LOG_CONTEXT_CF(Instance);
    nngn::destroy(&this->surface, vkDestroySurfaceKHR, this->id);
    if(this->messenger)
        this->call<PFN_vkDestroyDebugUtilsMessengerEXT>(
            "vkDestroyDebugUtilsMessengerEXT",
            this->id, this->messenger, nullptr);
    this->set_obj_name_fp = nullptr;
    nngn::destroy(&this->id, vkDestroyInstance);
}

bool Instance::set_obj_name(
        VkDevice dev, VkObjectType type,
        uint64_t obj, std::string_view name) const {
    if(!this->set_obj_name_fp)
        return true;
    VkDebugUtilsObjectNameInfoEXT info = {};
    info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    info.objectType = type;
    info.objectHandle = obj;
    info.pObjectName = name.data();
    CHECK_RESULT(this->set_obj_name_fp, dev, &info);
    return true;
}

bool Instance::set_obj_name_n(
        VkDevice dev, VkObjectType type,
        uint64_t obj, std::string_view name, size_t n) const {
    if(!this->set_obj_name_fp)
        return true;
    return this->set_obj_name(
        dev, type, obj, (name.data() + std::to_string(n)).c_str());
}

template<typename T> bool Instance::set_obj_name(
        VkDevice dev, T obj, std::string_view name) const {
    if constexpr(std::is_base_of_v<Handle, T>)
        return this->set_obj_name(dev, obj_type<T>, obj.id, name);
    else
        return this->set_obj_name(
            dev, obj_type<T>, reinterpret_cast<uintptr_t>(obj), name);
}

template<typename T> bool Instance::set_obj_name_n(
    VkDevice dev, T obj, std::string_view name, size_t n) const {
    if constexpr(std::is_base_of_v<Handle, T>)
        return this->set_obj_name_n(dev, obj_type<T>, obj.id, name, n);
    else
        return this->set_obj_name_n(
            dev, obj_type<T>, reinterpret_cast<uintptr_t>(obj), name, n);
}

bool PhysicalDevice::create_device(
        const Instance &inst, uint32_t queue_family, Device *dev) const {
    NNGN_LOG_CONTEXT_CF(PhysicalDevice);
    constexpr float priority = 1;
    VkDeviceQueueCreateInfo queue_info = {};
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.queueFamilyIndex = queue_family;
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = &priority;
    VkPhysicalDeviceFeatures features = {};
    features.fillModeNonSolid = VK_TRUE;
    features.imageCubeArray = VK_TRUE;
    VkDeviceCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    info.queueCreateInfoCount = 1;
    info.pQueueCreateInfos = &queue_info;
    info.pEnabledFeatures = &features;
    info.enabledExtensionCount = 1;
    info.ppEnabledExtensionNames = &SWAPCHAIN_EXTENSION;
    info.enabledLayerCount = VALIDATION_LAYERS.size();
    info.ppEnabledLayerNames = VALIDATION_LAYERS.data();
    CHECK_RESULT(vkCreateDevice, this->id, &info, nullptr, &dev->id);
    return dev->init(inst, queue_family);
}

void PhysicalDevice::update_surface_cap(VkSurfaceKHR surface) {
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        this->id, surface, &this->surface_cap);
}

void DeviceMemory::init(VkPhysicalDevice physical_dev) {
    vkGetPhysicalDeviceMemoryProperties(physical_dev, &this->props);
}

void DeviceMemory::destroy() {
    NNGN_LOG_CONTEXT_CF(DeviceMemory)
    for(auto x : this->allocs)
        Log::l() << "device memory was not deallocated: " << x << std::endl;
    this->allocs.clear();
}

bool DeviceMemory::find_type(
        uint32_t type, VkMemoryPropertyFlags f, uint32_t *ret) const {
    for(uint32_t i = 0; i < this->props.memoryTypeCount; ++i)
        if(type & (1u << i) && this->props.memoryTypes[i].propertyFlags & f) {
            *ret = i;
            return true;
        }
    Log::l() << "DeviceMemory::find_type: no suitable type found" << std::endl;
    return false;
}

bool DeviceMemory::alloc(
        VkDevice dev, VkMemoryRequirements req, VkMemoryPropertyFlags f,
        VkDeviceMemory *ret) {
    uint32_t type = {};
    if(!this->find_type(req.memoryTypeBits, f, &type))
        return false;
    VkMemoryAllocateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    info.allocationSize = req.size;
    info.memoryTypeIndex = type;
    CHECK_RESULT(vkAllocateMemory, dev, &info, nullptr, ret);
    this->allocs.push_back(*ret);
    return true;
}

bool DeviceMemory::alloc(
        VkDevice dev, VkBuffer buf, VkMemoryPropertyFlags f,
        VkDeviceMemory *ret) {
    VkMemoryRequirements req;
    vkGetBufferMemoryRequirements(dev, buf, &req);
    return this->alloc(dev, req, f, ret);
}

bool DeviceMemory::alloc(
        VkDevice dev, VkImage img, VkMemoryPropertyFlags f,
        VkDeviceMemory *ret) {
    VkMemoryRequirements req;
    vkGetImageMemoryRequirements(dev, img, &req);
    return this->alloc(dev, req, f, ret);
}

void DeviceMemory::dealloc(VkDevice dev, VkDeviceMemory mem) {
    const auto b = this->allocs.cbegin(), e = this->allocs.cend();
    const auto it = std::find(b, e, mem);
    assert(it != e);
    vkFreeMemory(dev, *it, nullptr);
    this->allocs.erase(it);
}

bool CommandPool::init(
        VkDevice dev, VkCommandPoolCreateFlags flags, uint32_t queue_family) {
    VkCommandPoolCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.flags = flags;
    info.queueFamilyIndex = queue_family;
    CHECK_RESULT(vkCreateCommandPool, dev, &info, nullptr, &this->id);
    return true;
}

void CommandPool::destroy(VkDevice dev)
    { nngn::destroy(&this->id, vkDestroyCommandPool, dev); }

template<typename F> bool CommandPool::with_cmd_buffer(
        VkDevice d, VkQueue queue, F f) const {
    NNGN_LOG_CONTEXT_CF(CommandPool);
    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandPool = this->id;
    alloc_info.commandBufferCount = 1;
    VkCommandBuffer b = {};
    vkAllocateCommandBuffers(d, &alloc_info, &b);
    const auto fin = make_scoped(
        [d, p = this->id, b] { vkFreeCommandBuffers(d, p, 1, &b); });
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(b, &begin_info);
    if(!f(b))
        return false;
    vkEndCommandBuffer(b);
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &b;
    CHECK_RESULT(vkQueueSubmit, queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);
    return true;
}

bool DescriptorPool::init(
        VkDevice dev, uint32_t max,
        uint32_t n_sizes, const VkDescriptorPoolSize *sizes) {
    NNGN_LOG_CONTEXT_CF(DescriptorPool);
    VkDescriptorPoolCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.poolSizeCount = n_sizes;
    info.pPoolSizes = sizes;
    info.maxSets = max;
    CHECK_RESULT(vkCreateDescriptorPool, dev, &info, nullptr, &this->id);
    return true;
}

void DescriptorPool::destroy(VkDevice dev)
    { nngn::destroy(&this->id, vkDestroyDescriptorPool, dev); }

bool DescriptorSets::init(
        VkDevice dev, uint32_t max,
        uint32_t n_bindings, const VkDescriptorSetLayoutBinding *bindings) {
    NNGN_LOG_CONTEXT_CF(DescriptorSets);
    VkDescriptorSetLayoutCreateInfo layout_info = {};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = n_bindings;
    layout_info.pBindings = bindings;
    CHECK_RESULT(vkCreateDescriptorSetLayout,
        dev, &layout_info, nullptr, &this->layout);
    std::vector<VkDescriptorPoolSize> sizes;
    sizes.reserve(n_bindings);
    for(size_t i = 0; i < n_bindings; ++i)
        sizes.push_back({bindings[i].descriptorType, max});
    if(!this->pool.init(
            dev, max, static_cast<uint32_t>(sizes.size()), sizes.data()))
        return false;
    std::vector<VkDescriptorSetLayout> layouts(max, this->layout);
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = this->pool.id;
    alloc_info.descriptorSetCount = max;
    alloc_info.pSetLayouts = layouts.data();
    this->ids.resize(max);
    CHECK_RESULT(vkAllocateDescriptorSets, dev, &alloc_info, this->ids.data());
    return true;
}

void DescriptorSets::destroy(VkDevice dev) {
    nngn::destroy(&this->layout, vkDestroyDescriptorSetLayout, dev);
    this->pool.destroy(dev);
    this->ids.clear();
}

bool CameraDescriptorSets::init(VkDevice d, uint32_t max) {
    NNGN_LOG_CONTEXT_CF(CameraDescriptorSets);
    const uint32_t n = max * 2
        + 7 * static_cast<uint32_t>(nngn::Lighting::MAX_LIGHTS);
    VkDescriptorSetLayoutBinding binding = {};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    return DescriptorSets::init(d, n, 1, &binding);
}

bool CameraDescriptorSets::write(
        VkDevice dev, uint32_t max,
        const Buffer *ubos, const Buffer *hud_ubos,
        const Buffer *shadow_ubos) const {
    const uint32_t n = max * 2
        + 7 * static_cast<uint32_t>(nngn::Lighting::MAX_LIGHTS);
    std::vector<VkDescriptorBufferInfo> buffer_infos;
    std::vector<VkWriteDescriptorSet> writes;
    buffer_infos.reserve(n);
    writes.reserve(n);
    const auto add_write = [&buffer_infos, &writes](auto b, auto s) {
        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = b;
        buffer_info.offset = 0;
        buffer_info.range = sizeof(CameraUBO);
        buffer_infos.push_back(buffer_info);
        VkWriteDescriptorSet write = {};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = s;
        write.dstBinding = 0;
        write.dstArrayElement = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.descriptorCount = 1;
        write.pBufferInfo = &buffer_infos.back();
        writes.push_back(write);
    };
    const auto *id = this->ids.data();
    for(size_t i = 0; i < max; ++i)
        add_write(ubos[i].id, *id++);
    for(size_t i = 0; i < max; ++i)
        add_write(hud_ubos[i].id, *id++);
    for(size_t i = 0; i < nngn::Lighting::MAX_LIGHTS; ++i)
        add_write(shadow_ubos[i].id, *id++);
    for(size_t i = 0; i < 6 * nngn::Lighting::MAX_LIGHTS; ++i)
        add_write(shadow_ubos[nngn::Lighting::MAX_LIGHTS + i].id, *id++);
    vkUpdateDescriptorSets(
        dev, static_cast<uint32_t>(writes.size()),
        writes.data(), 0, nullptr);
    return true;
}

bool LightingDescriptorSets::init(VkDevice d, uint32_t max) {
    NNGN_LOG_CONTEXT_CF(LightingDescriptorSets);
    std::array<VkDescriptorSetLayoutBinding, 3> bindings = {};
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags =
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[2].binding = 2;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[2].descriptorCount = 1;
    bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    return DescriptorSets::init(d, 1 + max, bindings.size(), bindings.data());
}

bool LightingDescriptorSets::write(
        VkDevice dev, uint32_t max, const Buffer *ubos,
        VkSampler sampler,
        VkImageView shadow_map_view, VkImageView shadow_cube_view) const {
    std::array<VkDescriptorImageInfo, 2> img_infos = {};
    img_infos[0].imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    img_infos[0].imageView = shadow_map_view;
    img_infos[0].sampler = sampler;
    img_infos[1].imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    img_infos[1].imageView = shadow_cube_view;
    img_infos[1].sampler = sampler;
    std::vector<VkDescriptorBufferInfo> buffer_infos;
    std::vector<VkWriteDescriptorSet> writes;
    const auto n = 1 + max;
    buffer_infos.reserve(n);
    writes.reserve(n * 3);
    for(auto i = 0u; i < n; ++i) {
        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = ubos[i].id;
        buffer_info.offset = 0;
        buffer_info.range = sizeof(LightsUBO);
        buffer_infos.push_back(buffer_info);
        VkWriteDescriptorSet write = {};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = this->ids[i];
        write.dstBinding = 0;
        write.dstArrayElement = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.descriptorCount = 1;
        write.pBufferInfo = &buffer_infos[i];
        writes.push_back(write);
        write.dstBinding = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.pImageInfo = img_infos.data();
        writes.push_back(write);
        write.dstBinding = 2;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.pImageInfo = img_infos.data() + 1;
        writes.push_back(write);
    }
    vkUpdateDescriptorSets(
        dev, static_cast<uint32_t>(writes.size()),
        writes.data(), 0, nullptr);
    return true;
}

bool TextureDescriptorSets::init(VkDevice dev) {
    NNGN_LOG_CONTEXT_CF(TextureDescriptorSets);
    VkDescriptorSetLayoutBinding binding = {};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    return DescriptorSets::init(dev, 4, 1, &binding);
}

bool TextureDescriptorSets::write(
        VkDevice dev, VkSampler tex_sampler, VkSampler shadow_sampler,
        VkImageView tex_img_view, VkImageView font_img_view,
        VkImageView shadow_map_img_view,
        VkImageView shadow_cube_img_view) const {
    std::array<VkDescriptorImageInfo, 4> img_infos = {};
    std::array<VkWriteDescriptorSet, 4> writes = {};
    uint32_t i = 0;
    const auto add_write = [&img_infos, &writes, &i](
            auto id, auto img_view, auto sampler, bool depth) {
        if(!img_view || !sampler)
            return;
        img_infos[i].imageLayout = depth
            ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
            : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        img_infos[i].imageView = img_view;
        img_infos[i].sampler = sampler;
        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = id;
        writes[i].dstBinding = 0;
        writes[i].dstArrayElement = 0;
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[i].descriptorCount = 1;
        writes[i].pImageInfo = &img_infos[i];
        ++i;
    };
    add_write(this->ids[0], tex_img_view, tex_sampler, false);
    add_write(this->ids[1], font_img_view, tex_sampler, false);
    add_write(this->ids[2], shadow_map_img_view, shadow_sampler, true);
    add_write(this->ids[3], shadow_cube_img_view, shadow_sampler, true);
    vkUpdateDescriptorSets(dev, i, writes.data(), 0, nullptr);
    return true;
}

void Image::destroy(DeviceMemory *dev_mem, VkDevice dev) {
    nngn::destroy(&this->id, vkDestroyImage, dev);
    if(this->mem)
        dev_mem->dealloc(dev, this->mem);
    this->mem = VK_NULL_HANDLE;
}

bool Image::transition_layout(
        VkDevice dev, VkQueue queue, const CommandPool &cmd_pool,
        VkImageAspectFlags aspect, VkImageLayout src, VkImageLayout dst,
        uint32_t mip_levels, uint32_t base_layer, uint32_t n_layers) const {
    NNGN_LOG_CONTEXT_CF(Image);
    return cmd_pool.with_cmd_buffer(
        dev, queue,
        [this, aspect, src, dst, mip_levels, base_layer, n_layers](auto b) {
            VkImageMemoryBarrier barrier = {};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = src;
            barrier.newLayout = dst;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = this->id;
            barrier.subresourceRange.aspectMask = aspect;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = mip_levels;
            barrier.subresourceRange.baseArrayLayer = base_layer;
            barrier.subresourceRange.layerCount = n_layers;
            VkPipelineStageFlags src_stage = {}, dst_stage = {};
            constexpr auto
                undefined = VK_IMAGE_LAYOUT_UNDEFINED,
                dstop = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                sread = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                dread = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            if((src == undefined && dst == dstop)
                   || (src == undefined && dst == sread)
                   || (src == undefined && dst == dread)
                   || (src == dstop && dst == sread)) {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            } else {
                Log::l()
                    << "unsupported layout transition "
                    << src << " -> " << dst << std::endl;
                return false;
            }
            vkCmdPipelineBarrier(
                b, src_stage, dst_stage, 0,
                0, nullptr, 0, nullptr, 1, &barrier);
            return true;
        });
}

bool Swapchain::init(
        const Instance &inst, DeviceMemory *dev_mem, Device *dev,
        const VkExtent2D &extent,
        VkExtent2D shadow_map_size, VkExtent2D shadow_cube_size,
        const TexArray &shadow_map, const TexArray &shadow_cube) {
    NNGN_LOG_CONTEXT_CF(Swapchain);
    return this->create_depth_img(inst, dev_mem, *dev, extent)
        && this->create_img_views(inst, dev)
        && this->create_render_pass(inst, dev->id)
        && this->create_depth_pass(
            inst, dev->id,
            shadow_map_size, shadow_cube_size, shadow_map, shadow_cube)
        && this->create_descriptor_set_layout(inst, dev->id)
        && this->create_graphics_pipelines(
            inst, dev, extent, shadow_map_size, shadow_cube_size)
        && this->create_framebuffers(inst, dev->id, extent)
        && this->create_sync_objects(inst, dev->id);
}

bool Swapchain::recreate(
        const Instance &inst, DeviceMemory *dev_mem, Device *dev,
        const VkExtent2D &extent,
        VkExtent2D shadow_map_size, VkExtent2D shadow_cube_size,
        const TexArray &shadow_map, const TexArray &shadow_cube) {
    NNGN_LOG_CONTEXT_CF(Swapchain);
    return this->create_depth_img(inst, dev_mem, *dev, extent)
        && this->create_img_views(inst, dev)
        && this->create_render_pass(inst, dev->id)
        && this->create_depth_pass(
            inst, dev->id,
            shadow_map_size, shadow_cube_size, shadow_map, shadow_cube)
        && this->create_graphics_pipelines(
            inst, dev, extent, shadow_map_size, shadow_cube_size)
        && this->create_framebuffers(inst, dev->id, extent);
}

void Swapchain::soft_destroy(DeviceMemory *dev_mem, VkDevice dev) {
    NNGN_LOG_CONTEXT_CF(Swapchain);
    for(auto *v : {&this->framebuffers, &this->depth_framebuffers}) {
        for(auto &x : *v)
            nngn::destroy(&x, vkDestroyFramebuffer, dev);
        v->clear();
    }
    for(auto *x : {
            &this->triangle_pipeline, &this->sprite_pipeline,
            &this->triangle_depth_pipeline[0], &this->sprite_depth_pipeline[0],
            &this->triangle_depth_pipeline[1], &this->sprite_depth_pipeline[1],
            &this->box_pipeline, &this->font_pipeline, &this->grid_pipeline,
            &this->circle_pipeline, &this->wire_pipeline,
            &this->map_ortho_pipeline, &this->map_persp_pipeline,
            &this->map_depth_pipeline[0], &this->map_depth_pipeline[1]})
        nngn::destroy(x, vkDestroyPipeline, dev);
    for(auto *x : {
            &this->render_pipeline_layout,
            &this->depth_triangle_pipeline_layout,
            &this->depth_sprite_pipeline_layout})
        nngn::destroy(x, vkDestroyPipelineLayout, dev);
    for(auto *x : {&this->render_pass, &this->depth_pass})
        nngn::destroy(x, vkDestroyRenderPass, dev);
    this->depth_img.destroy(dev_mem, dev);
    nngn::destroy(&this->depth_img_view, vkDestroyImageView, dev);
    for(auto &x : this->img_views)
        nngn::destroy(&x, vkDestroyImageView, dev);
    this->img_views.clear();
    nngn::destroy(&this->id, vkDestroySwapchainKHR, dev);
}

void Swapchain::destroy(DeviceMemory *dev_mem, VkDevice dev) {
    NNGN_LOG_CONTEXT_CF(Swapchain);
    if(!this->id)
        return;
    for(auto &x : this->image_available_sem)
        nngn::destroy(&x, vkDestroySemaphore, dev);
    for(auto &x : this->render_finished_sem)
        nngn::destroy(&x, vkDestroySemaphore, dev);
    for(auto &x : this->fences)
        nngn::destroy(&x, vkDestroyFence, dev);
    this->camera_descriptor_sets.destroy(dev);
    this->lighting_descriptor_sets.destroy(dev);
    this->texture_descriptor_sets.destroy(dev);
    this->soft_destroy(dev_mem, dev);
}

bool Swapchain::create_img_views(const Instance &inst, Device *dev) {
    NNGN_LOG_CONTEXT_CF(Swapchain);
    const auto imgs = enumerate<VkImage>(
        vkGetSwapchainImagesKHR, dev->id, this->id);
    this->img_views.resize(imgs.size());
    for(size_t i = 0, n = imgs.size(); i < n; ++i) {
        if(!inst.set_obj_name_n(dev->id, imgs[i], "swapchain_img"sv, i))
            return false;
        if(!dev->create_img_view(
                imgs[i], SURFACE_FORMAT, VK_IMAGE_VIEW_TYPE_2D,
                VK_IMAGE_ASPECT_COLOR_BIT, 1, 0, 1, &this->img_views[i]))
            return false;
        if(!inst.set_obj_name_n(
                dev->id, this->img_views[i], "swapchain_img_view"sv, i))
            return false;
    }
    return true;
}

bool Swapchain::create_depth_img(
        const Instance &inst, DeviceMemory *dev_mem, const Device &dev,
        const VkExtent2D &extent) {
    NNGN_LOG_CONTEXT_CF(Swapchain);
    return dev.create_img(
            dev_mem, extent, 1, 1,
            DEPTH_FORMAT, VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_SAMPLE_COUNT_1_BIT,
            0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &this->depth_img)
        && inst.set_obj_name(dev.id, this->depth_img.id, "depth_img"sv)
        && inst.set_obj_name(dev.id, this->depth_img.mem, "depth_img_mem"sv)
        && dev.create_img_view(
            this->depth_img.id, DEPTH_FORMAT,
            VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_DEPTH_BIT, 1, 0, 1,
            &this->depth_img_view)
        && inst.set_obj_name(dev.id, this->depth_img_view, "depth_img_view"sv);
}

bool Swapchain::create_depth_pass(
        const Instance &inst, VkDevice dev,
        VkExtent2D shadow_map_size, VkExtent2D shadow_cube_size,
        const TexArray &shadow_map, const TexArray &shadow_cube) {
    VkAttachmentDescription depth_att = {};
    depth_att.format = DEPTH_FORMAT;
    depth_att.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_att.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_att.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depth_att.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_att.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_att.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_att.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    VkAttachmentReference depth_att_ref = {};
    depth_att_ref.attachment = 0;
    depth_att_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 0;
    subpass.pDepthStencilAttachment = &depth_att_ref;
    std::array<VkSubpassDependency, 2> deps = {};
    deps[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    deps[0].dstSubpass = 0;
    deps[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    deps[0].srcAccessMask = 0;
    deps[0].dstStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    deps[0].dstAccessMask =
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
        | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    deps[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    deps[1].srcSubpass = 0;
    deps[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    deps[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    deps[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
        | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    deps[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    deps[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    deps[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    VkRenderPassCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    info.attachmentCount = 1;
    info.pAttachments = &depth_att;
    info.subpassCount = 1;
    info.pSubpasses = &subpass;
    info.dependencyCount = deps.size();
    info.pDependencies = deps.data();
    CHECK_RESULT(vkCreateRenderPass,
        dev, &info, nullptr, &this->depth_pass);
    if(!inst.set_obj_name(dev, this->depth_pass, "depth_pass"sv))
        return false;
    this->depth_framebuffers.resize(7 * nngn::Lighting::MAX_LIGHTS);
    VkFramebufferCreateInfo fb_info = {};
    fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fb_info.renderPass = this->depth_pass;
    fb_info.attachmentCount = 1;
    fb_info.width = shadow_map_size.width;
    fb_info.height = shadow_map_size.height;
    fb_info.layers = 1;
    for(size_t i = 0; i < nngn::Lighting::MAX_LIGHTS; ++i) {
        fb_info.pAttachments = &shadow_map.layer_views[i];
        CHECK_RESULT(vkCreateFramebuffer,
            dev, &fb_info, nullptr, &this->depth_framebuffers[i]);
        inst.set_obj_name_n(
            dev, this->depth_framebuffers[i],
            "depth_framebuffers"sv, i);
    }
    fb_info.width = shadow_cube_size.width;
    fb_info.height = shadow_cube_size.height;
    for(size_t i = 0; i < 6 * nngn::Lighting::MAX_LIGHTS; ++i) {
        fb_info.pAttachments = &shadow_cube.layer_views[i];
        CHECK_RESULT(vkCreateFramebuffer,
            dev, &fb_info, nullptr,
            &this->depth_framebuffers[nngn::Lighting::MAX_LIGHTS + i]);
        inst.set_obj_name_n(
            dev, this->depth_framebuffers[i],
            "depth_framebuffers"sv, nngn::Lighting::MAX_LIGHTS + i);
    }
    return true;
}

bool Swapchain::create_render_pass(const Instance &inst, VkDevice dev) {
    NNGN_LOG_CONTEXT_CF(Swapchain);
    VkAttachmentDescription color_att = {};
    color_att.format = SURFACE_FORMAT;
    color_att.samples = VK_SAMPLE_COUNT_1_BIT;
    color_att.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_att.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_att.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_att.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_att.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_att.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    VkAttachmentReference color_att_ref = {};
    color_att_ref.attachment = 0;
    color_att_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VkAttachmentDescription depth_att = {};
    depth_att.format = DEPTH_FORMAT;
    depth_att.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_att.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_att.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_att.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_att.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_att.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_att.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    VkAttachmentReference depth_att_ref = {};
    depth_att_ref.attachment = 1;
    depth_att_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_att_ref;
    subpass.pDepthStencilAttachment = &depth_att_ref;
    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
        | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    std::array<VkAttachmentDescription, 2> attachments =
        {color_att, depth_att};
    VkRenderPassCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    info.attachmentCount = attachments.size();
    info.pAttachments = attachments.data();
    info.subpassCount = 1;
    info.pSubpasses = &subpass;
    info.dependencyCount = 1;
    info.pDependencies = &dependency;
    CHECK_RESULT(vkCreateRenderPass, dev, &info, nullptr, &this->render_pass);
    return inst.set_obj_name(dev, this->render_pass, "main_render_pass"sv);
}

bool Swapchain::create_descriptor_set_layout(
        const Instance &inst, VkDevice dev) {
    const auto name = [&inst, dev](auto p, auto n) {
        if(!inst.set_obj_name(dev, p->layout, n + "_descriptor_set_layout"s))
            return false;
        if(!inst.set_obj_name(dev, p->pool.id, n + "_descriptor_pool"s))
            return false;
        for(size_t i = 0; i < p->ids.size(); ++i)
            if(!inst.set_obj_name_n(
                    dev, p->ids[i], n + "_descriptor_set"s, i))
                return false;
        return true;
    };
    const auto n = static_cast<uint32_t>(this->img_views.size());
    return this->camera_descriptor_sets.init(dev, n)
        && name(&this->camera_descriptor_sets, "camera")
        && this->lighting_descriptor_sets.init(dev, n)
        && name(&this->lighting_descriptor_sets, "lighting")
        && this->texture_descriptor_sets.init(dev)
        && name(&this->texture_descriptor_sets, "texture");
}

bool Swapchain::create_graphics_pipelines(
        const Instance &inst, Device *dev, const VkExtent2D &extent,
        VkExtent2D shadow_map_size, VkExtent2D shadow_cube_size) {
    NNGN_LOG_CONTEXT_CF(Swapchain);
    VkPipelineMultisampleStateCreateInfo multisampling_info = {};
    multisampling_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling_info.sampleShadingEnable = VK_FALSE;
    multisampling_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    VkPipelineColorBlendAttachmentState color_blend_att = {};
    color_blend_att.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
        | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_att.blendEnable = VK_TRUE;
    color_blend_att.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_att.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_att.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_att.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_att.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_att.alphaBlendOp = VK_BLEND_OP_ADD;
    VkPipelineColorBlendStateCreateInfo color_blending_info = {};
    color_blending_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending_info.logicOpEnable = VK_FALSE;
    color_blending_info.attachmentCount = 1;
    color_blending_info.pAttachments = &color_blend_att;
    std::array<VkDescriptorSetLayout, 3> descriptor_set_layouts = {
        this->lighting_descriptor_sets.layout,
        this->camera_descriptor_sets.layout,
        this->texture_descriptor_sets.layout};
    VkPushConstantRange push_const = {};
    push_const.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    push_const.offset = 0;
    push_const.size = sizeof(float);
    VkPipelineLayoutCreateInfo layout_info = {};
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.setLayoutCount = descriptor_set_layouts.size();
    layout_info.pSetLayouts = descriptor_set_layouts.data();
    layout_info.pushConstantRangeCount = 1;
    layout_info.pPushConstantRanges = &push_const;
    CHECK_RESULT(vkCreatePipelineLayout,
        dev->id, &layout_info, nullptr, &this->render_pipeline_layout);
    if(!inst.set_obj_name(
            dev->id, this->render_pipeline_layout, "render_pipeline_layout"sv))
        return false;
    //layout_info.pushConstantRangeCount = 0;
    CHECK_RESULT(vkCreatePipelineLayout,
        dev->id, &layout_info, nullptr, &this->depth_triangle_pipeline_layout);
    if(!inst.set_obj_name(
            dev->id, this->depth_triangle_pipeline_layout,
            "depth_triangle_pipeline_layout"sv))
        return false;
    CHECK_RESULT(vkCreatePipelineLayout,
        dev->id, &layout_info, nullptr, &this->depth_sprite_pipeline_layout);
    if(!inst.set_obj_name(
            dev->id, this->depth_sprite_pipeline_layout,
            "depth_sprite_pipeline_layout"sv))
        return false;
    VkGraphicsPipelineCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    info.flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
    info.stageCount = 2;
    info.pMultisampleState = &multisampling_info;
    info.pColorBlendState = &color_blending_info;
    info.layout = this->render_pipeline_layout;
    info.renderPass = this->render_pass;
    info.subpass = 0;
    std::array<std::tuple<VkShaderModule, VkShaderModule>, 5> shaders = {};
    const auto destroy_shaders = make_scoped([d = dev->id, &shaders]() {
        for(auto [v, f] : shaders) {
            vkDestroyShaderModule(d, v, nullptr);
            vkDestroyShaderModule(d, f, nullptr);
        }
    });
    auto &[triangle, sprite, font, triangle_depth, sprite_depth] = shaders;
    const auto create_shader_pair = [&inst, dev](
            auto *t, auto vs_name, auto fs_name, auto vs, auto fs) {
        auto &[v, f] = *t;
        return dev->create_shader(inst, vs_name, Shaders::load(vs), &v)
            && dev->create_shader(inst, fs_name, Shaders::load(fs), &f);
    };
    bool ok = true;
    ok = ok && create_shader_pair(&triangle,
        "src/glsl/vk/triangle.vert.spv", "src/glsl/vk/triangle.frag.spv",
        Shaders::Name::VK_TRIANGLE_VERT, Shaders::Name::VK_TRIANGLE_FRAG);
    ok = ok && create_shader_pair(&sprite,
        "src/glsl/vk/sprite.vert.spv", "src/glsl/vk/sprite.frag.spv",
        Shaders::Name::VK_SPRITE_VERT, Shaders::Name::VK_SPRITE_FRAG);
    ok = ok && create_shader_pair(&font,
        "src/glsl/vk/font.vert.spv", "src/glsl/vk/font.frag.spv",
        Shaders::Name::VK_FONT_VERT, Shaders::Name::VK_FONT_FRAG);
    ok = ok && create_shader_pair(&triangle_depth,
        "src/glsl/vk/triangle_depth.vert.spv",
        "src/glsl/vk/triangle_depth.frag.spv",
        Shaders::Name::VK_TRIANGLE_DEPTH_VERT,
        Shaders::Name::VK_TRIANGLE_DEPTH_FRAG);
    ok = ok && create_shader_pair(&sprite_depth,
        "src/glsl/vk/sprite_depth.vert.spv",
        "src/glsl/vk/sprite_depth.frag.spv",
        Shaders::Name::VK_SPRITE_DEPTH_VERT,
        Shaders::Name::VK_SPRITE_DEPTH_FRAG);
    if(!ok)
        return false;
    const auto create_stages = [](const auto &s) {
        const auto [v, f] = s;
        std::array<VkPipelineShaderStageCreateInfo, 2> ret = {};
        ret[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        ret[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        ret[0].pName = "main";
        ret[0].module = v;
        ret[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        ret[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        ret[1].pName = "main";
        ret[1].module = f;
        return ret;
    };
    const auto triangle_stages = create_stages(triangle);
    const auto sprite_stages = create_stages(sprite);
    const auto font_stages = create_stages(font);
    const auto triangle_depth_stages = create_stages(triangle_depth);
    const auto sprite_depth_stages = create_stages(sprite_depth);
    const auto create_vinput = [](
            const uint32_t *offset0, const uint32_t *offset1,
            const uint32_t *offset2) {
        struct {
            VkPipelineVertexInputStateCreateInfo info = {};
            VkVertexInputBindingDescription bdesc = {};
            uint32_t n_adesc = 0;
            std::array<VkVertexInputAttributeDescription, 3> adesc = {};
        } ret = {};
        ret.bdesc.binding = 0;
        ret.bdesc.stride = sizeof(Vertex);
        ret.bdesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        const auto fill = [&ret](auto *p, uint32_t i) {
            if(!p)
                return;
            ++ret.n_adesc;
            ret.adesc[i].binding = 0;
            ret.adesc[i].location = i;
            ret.adesc[i].format = VK_FORMAT_R32G32B32_SFLOAT;
            ret.adesc[i].offset = *p;
        };
        fill(offset0, 0);
        fill(offset1, 1);
        fill(offset2, 2);
        ret.info.sType =
            VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        ret.info.vertexBindingDescriptionCount = 1;
        ret.info.pVertexBindingDescriptions = &ret.bdesc;
        ret.info.vertexAttributeDescriptionCount = ret.n_adesc;
        ret.info.pVertexAttributeDescriptions = ret.adesc.data();
        return ret;
    };
    constexpr uint32_t off_pos = offsetof(Vertex, pos);
    constexpr uint32_t off_norm = offsetof(Vertex, norm);
    constexpr uint32_t off_color = offsetof(Vertex, color);
    const auto vertex_vinput = create_vinput(&off_pos, &off_norm, &off_color);
    const auto nonorm_vinput = create_vinput(&off_pos, &off_color, nullptr);
    const auto pos_vinput = create_vinput(&off_pos, nullptr, nullptr);
    const auto create_depth_stencil = [](bool test_write) {
        VkPipelineDepthStencilStateCreateInfo ret = {};
        ret.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        ret.depthTestEnable = test_write;
        ret.depthWriteEnable = test_write;
        ret.depthCompareOp = VK_COMPARE_OP_LESS;
        ret.depthBoundsTestEnable = VK_FALSE;
        ret.stencilTestEnable = VK_FALSE;
        return ret;
    };
    const auto depth_stencil = create_depth_stencil(true);
    const auto no_depth_stencil = create_depth_stencil(false);
    const auto create_rasterization_state = [](
            auto cull_mode, auto polygon_mode) {
        VkPipelineRasterizationStateCreateInfo ret = {};
        ret.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        ret.depthClampEnable = VK_FALSE;
        ret.rasterizerDiscardEnable = VK_FALSE;
        ret.polygonMode = polygon_mode;
        ret.lineWidth = 1;
        ret.cullMode = cull_mode;
        ret.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        ret.depthBiasEnable = VK_FALSE;
        return ret;
    };
    auto rast_back_cull = create_rasterization_state(
        VK_CULL_MODE_BACK_BIT, VK_POLYGON_MODE_FILL);
    auto rast_no_cull = create_rasterization_state(
        VK_CULL_MODE_NONE, VK_POLYGON_MODE_FILL);
    auto rast_line = create_rasterization_state(
        VK_CULL_MODE_NONE, VK_POLYGON_MODE_LINE);
    const auto create_asm_info = [](VkPrimitiveTopology t) {
        VkPipelineInputAssemblyStateCreateInfo ret = {};
        ret.sType =
            VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        ret.topology = t;
        ret.primitiveRestartEnable = VK_FALSE;
        return ret;
    };
    const auto triangle_asm =
        create_asm_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    const auto line_asm =
        create_asm_info(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
    const auto create_viewport = [](VkExtent2D ex) {
        VkViewport ret = {};
        ret.x = 0;
        ret.y = 0;
        ret.width = static_cast<float>(ex.width);
        ret.height = static_cast<float>(ex.height);
        ret.minDepth = 0;
        ret.maxDepth = 1;
        return ret;
    };
    const std::array<VkViewport, 3> viewports = {{
        create_viewport(extent),
        create_viewport(shadow_map_size),
        create_viewport(shadow_cube_size)}};
    const auto create_scissors = [](VkExtent2D ex) {
        VkRect2D ret = {};
        ret.offset = {0, 0};
        ret.extent = ex;
        return ret;
    };
    const std::array<VkRect2D, 3> scissors = {{
        create_scissors(extent),
        create_scissors(shadow_map_size),
        create_scissors(shadow_cube_size)}};
    const auto create_viewport_state = [](
            const auto &viewport, const auto &sc) {
        VkPipelineViewportStateCreateInfo ret = {};
        ret.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        ret.viewportCount = 1;
        ret.pViewports = &viewport;
        ret.scissorCount = 1;
        ret.pScissors = &sc;
        return ret;
    };
    const std::array<VkPipelineViewportStateCreateInfo, 3> viewport_states = {{
        create_viewport_state(viewports[0], scissors[0]),
        create_viewport_state(viewports[1], scissors[1]),
        create_viewport_state(viewports[2], scissors[2])}};
    const auto create = [&inst, dev, &info](
            std::string_view n, auto *p, const auto &s, const auto &i,
            const auto &d, const auto &a, const auto &v, const auto &r, auto l,
            auto pass) {
        info.pStages = s.data();
        info.pVertexInputState = &i.info;
        info.pInputAssemblyState = &a;
        info.pViewportState = &v;
        info.pRasterizationState = &r;
        info.pDepthStencilState = &d;
        info.layout = l;
        info.renderPass = pass;
        CHECK_RESULT(vkCreateGraphicsPipelines,
            dev->id, VK_NULL_HANDLE, 1, &info, nullptr, p);
        return inst.set_obj_name(dev->id, *p, n);
    };
    ok = ok && create("triangle_pipeline"sv,
        &this->triangle_pipeline, triangle_stages, vertex_vinput,
        depth_stencil, triangle_asm, viewport_states[0], rast_back_cull,
        this->render_pipeline_layout, this->render_pass);
    ok = ok && create("sprite_pipeline"sv,
        &this->sprite_pipeline, sprite_stages, vertex_vinput,
        depth_stencil, triangle_asm, viewport_states[0], rast_no_cull,
        this->render_pipeline_layout, this->render_pass);
    ok = ok && create("circle_pipeline"sv,
        &this->circle_pipeline, sprite_stages, vertex_vinput,
        no_depth_stencil, triangle_asm, viewport_states[0], rast_no_cull,
        this->render_pipeline_layout, this->render_pass);
    ok = ok && create("font_pipeline"sv,
        &this->font_pipeline, font_stages, vertex_vinput,
        no_depth_stencil, triangle_asm, viewport_states[0], rast_no_cull,
        this->render_pipeline_layout, this->render_pass);
    ok = ok && create("box_pipeline"sv,
        &this->box_pipeline, triangle_stages, vertex_vinput,
        no_depth_stencil, triangle_asm, viewport_states[0], rast_no_cull,
        this->render_pipeline_layout, this->render_pass);
    ok = ok && create("grid_pipeline"sv,
        &this->grid_pipeline, triangle_stages, vertex_vinput,
        no_depth_stencil, line_asm, viewport_states[0], rast_no_cull,
        this->render_pipeline_layout, this->render_pass);
    ok = ok && create("map_ortho_pipeline"sv,
        &this->map_ortho_pipeline, sprite_stages, vertex_vinput,
        no_depth_stencil, triangle_asm, viewport_states[0], rast_no_cull,
        this->render_pipeline_layout, this->render_pass);
    ok = ok && create("map_persp_pipeline"sv,
        &this->map_persp_pipeline, sprite_stages, vertex_vinput,
        depth_stencil, triangle_asm, viewport_states[0], rast_no_cull,
        this->render_pipeline_layout, this->render_pass);
    ok = ok && create("wire_pipeline"sv,
        &this->wire_pipeline, triangle_stages, vertex_vinput,
        no_depth_stencil, line_asm, viewport_states[0], rast_line,
        this->render_pipeline_layout, this->render_pass);
    ok = ok && create("map_depth_pipeline0"sv,
        &this->map_depth_pipeline[0], sprite_depth_stages, nonorm_vinput,
        depth_stencil, triangle_asm, viewport_states[1], rast_no_cull,
        this->depth_sprite_pipeline_layout, this->depth_pass);
    ok = ok && create("map_depth_pipeline1"sv,
        &this->map_depth_pipeline[1], sprite_depth_stages, nonorm_vinput,
        depth_stencil, triangle_asm, viewport_states[2], rast_no_cull,
        this->depth_sprite_pipeline_layout, this->depth_pass);
    ok = ok && create("sprite_depth_pipeline0"sv,
        &this->sprite_depth_pipeline[0], sprite_depth_stages, nonorm_vinput,
        depth_stencil, triangle_asm, viewport_states[1], rast_no_cull,
        this->depth_sprite_pipeline_layout, this->depth_pass);
    ok = ok && create("sprite_depth_pipeline1"sv,
        &this->sprite_depth_pipeline[1], sprite_depth_stages, nonorm_vinput,
        depth_stencil, triangle_asm, viewport_states[2], rast_no_cull,
        this->depth_sprite_pipeline_layout, this->depth_pass);
    ok = ok && create("triangle_depth_pipeline0"sv,
        &this->triangle_depth_pipeline[0], triangle_depth_stages, pos_vinput,
        depth_stencil, triangle_asm, viewport_states[1], rast_no_cull,
        this->depth_triangle_pipeline_layout, this->depth_pass);
    ok = ok && create("triangle_depth_pipeline1"sv,
        &this->triangle_depth_pipeline[1], triangle_depth_stages, pos_vinput,
        depth_stencil, triangle_asm, viewport_states[2], rast_no_cull,
        this->depth_triangle_pipeline_layout, this->depth_pass);
    return ok;
}

bool Swapchain::create_framebuffers(
        const Instance &inst, VkDevice dev, const VkExtent2D &extent) {
    NNGN_LOG_CONTEXT_CF(Swapchain);
    const auto n = this->img_views.size();
    this->framebuffers.resize(n);
    std::array<VkImageView, 2> attachments = {};
    attachments[1] = this->depth_img_view;
    for(auto i = 0u; i < n; ++i) {
        attachments[0] = this->img_views[i];
        VkFramebufferCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        info.renderPass = this->render_pass;
        info.attachmentCount = attachments.size();
        info.pAttachments = attachments.data();
        info.width = extent.width;
        info.height = extent.height;
        info.layers = 1;
        CHECK_RESULT(vkCreateFramebuffer,
            dev, &info, nullptr, &this->framebuffers[i]);
        if(!inst.set_obj_name_n(
                dev, this->framebuffers[i], "swapchain_framebuffer"sv, i))
            return false;
    }
    return true;
}

bool Swapchain::create_sync_objects(const Instance &inst, VkDevice dev) {
    NNGN_LOG_CONTEXT_CF(Swapchain);
    const auto n = this->img_views.size();
    this->image_available_sem.resize(n);
    this->render_finished_sem.resize(n);
    this->fences.resize(n);
    VkSemaphoreCreateInfo sem_info = {};
    sem_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    for(auto &x : this->image_available_sem) {
        CHECK_RESULT(vkCreateSemaphore, dev, &sem_info, nullptr, &x);
        if(!inst.set_obj_name(dev, x, "image_available_sem"sv))
            return false;
    }
    for(auto &x : this->render_finished_sem) {
        CHECK_RESULT(vkCreateSemaphore, dev, &sem_info, nullptr, &x);
        if(!inst.set_obj_name(dev, x, "render_finished_sem"sv))
            return false;
    }
    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    for(auto &x : this->fences) {
        CHECK_RESULT(vkCreateFence, dev, &fence_info, nullptr, &x);
        if(!inst.set_obj_name(dev, x, "swapchain_fence"sv))
            return false;
    }
    return true;
}

bool Device::init(const Instance &inst, uint32_t queue_family) {
    vkGetDeviceQueue(this->id, queue_family, 0, &this->graphics_queue);
    this->present_queue = this->graphics_queue;
    return this->cmd_pool.init(this->id, 0, queue_family)
        && inst.set_obj_name(this->id, this->cmd_pool.id, "cmd_pool"sv)
        && this->tmp_cmd_pool.init(
            this->id, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, queue_family)
        && inst.set_obj_name(
            this->id, this->tmp_cmd_pool.id, "tmp_cmd_pool"sv);
}

void Device::destroy(DeviceMemory *dev_mem) {
    for(const auto &x : {&this->cmd_pool, &this->tmp_cmd_pool})
        x->destroy(this->id);
    this->swapchain.destroy(dev_mem, this->id);
    this->graphics_queue = this->present_queue = VK_NULL_HANDLE;
    nngn::destroy(&this->id, vkDestroyDevice);
}

bool Device::create_img(
        DeviceMemory *dev_mem,
        VkExtent2D extent, uint32_t mip_levels, uint32_t n_layers,
        VkFormat fmt, VkImageTiling tiling, VkImageUsageFlags usage,
        VkSampleCountFlagBits n_samples, VkImageCreateFlags flags,
        VkMemoryPropertyFlags mem_props, Image *img) const {
    NNGN_LOG_CONTEXT_CF(Device);
    VkImageCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.imageType = VK_IMAGE_TYPE_2D;
    info.extent = {extent.width, extent.height, 1};
    info.mipLevels = mip_levels;
    info.arrayLayers = n_layers;
    info.format = fmt;
    info.tiling = tiling;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    info.usage = usage;
    info.samples = n_samples;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.flags = flags;
    CHECK_RESULT(vkCreateImage, this->id, &info, nullptr, &img->id);
    if(!dev_mem->alloc(this->id, img->id, mem_props, &img->mem))
        return false;
    vkBindImageMemory(this->id, img->id, img->mem, 0);
    return true;
}

bool Device::create_img_view(
        VkImage img, VkFormat fmt, VkImageViewType type,
        VkImageAspectFlags aspect_flags,
        uint32_t mip_levels, uint32_t base_layer, uint32_t n_layers,
        VkImageView *img_view) const {
    NNGN_LOG_CONTEXT_CF(Device);
    VkImageViewCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.image = img;
    info.viewType = type;
    info.format = fmt;
    info.subresourceRange.aspectMask = aspect_flags;
    info.subresourceRange.baseMipLevel = 0;
    info.subresourceRange.levelCount = mip_levels;
    info.subresourceRange.baseArrayLayer = base_layer;
    info.subresourceRange.layerCount = n_layers;
    CHECK_RESULT(vkCreateImageView, this->id, &info, nullptr, img_view);
    return true;
}

bool Device::create_sampler(
        VkFilter filter, VkSamplerAddressMode addr_mode, VkBorderColor border,
        VkSamplerMipmapMode mip_mode, uint32_t mip_levels,
        VkSampler *sampler) const {
    VkSamplerCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.magFilter = filter;
    info.minFilter = filter;
    info.addressModeU = addr_mode;
    info.addressModeV = addr_mode;
    info.addressModeW = addr_mode;
    info.anisotropyEnable = VK_FALSE;
    info.maxAnisotropy = 1;
    info.borderColor = border;
    info.unnormalizedCoordinates = VK_FALSE;
    info.compareEnable = VK_FALSE;
    info.compareOp = VK_COMPARE_OP_ALWAYS;
    info.mipmapMode = mip_mode;
    info.maxLod = static_cast<float>(mip_levels);
    CHECK_RESULT(vkCreateSampler, this->id, &info, nullptr, sampler);
    return true;
}

bool Device::create_shader(
        const Instance &inst, std::string_view name, std::string_view src,
        VkShaderModule *m) const {
    NNGN_LOG_CONTEXT_CF(Device);
    NNGN_LOG_CONTEXT(name.data());
    VkShaderModuleCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = src.size();
    info.pCode = reinterpret_cast<const uint32_t*>(src.data());
    CHECK_RESULT(vkCreateShaderModule, this->id, &info, nullptr, m);
    return inst.set_obj_name(this->id, *m, name);
}

bool Buffer::init(
        VkDevice dev, DeviceMemory *dev_mem,
        VkBufferUsageFlags usage, VkMemoryPropertyFlags memp,
        VkDeviceSize sz) {
    NNGN_LOG_CONTEXT_CF(Buffer);
    assert(!this->id);
    VkBufferCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size = sz;
    info.usage = usage;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    CHECK_RESULT(vkCreateBuffer, dev, &info, nullptr, &this->id);
    if(!dev_mem->alloc(dev, this->id, memp, &this->mem))
        return false;
    vkBindBufferMemory(dev, this->id, this->mem, 0);
    this->capacity = sz;
    return true;
}

void Buffer::destroy(DeviceMemory *dev_mem, VkDevice dev) {
    nngn::destroy(&this->id, vkDestroyBuffer, dev);
    if(this->mem)
        dev_mem->dealloc(dev, this->mem);
    this->mem = VK_NULL_HANDLE;
    this->size = this->capacity = 0;
}

void Buffer::memcpy(VkDevice dev, const void *p, size_t sz) const {
    void *m = {};
    vkMapMemory(dev, this->mem, 0, sz, 0, &m);
    std::memcpy(m, p, sz);
    vkUnmapMemory(dev, this->mem);
}

bool TexArray::init(
        Device *dev, DeviceMemory *dev_mem,
        VkQueue queue, const CommandPool &cmd_pool,
        VkExtent2D extent, uint32_t mip_lvls, uint32_t n,
        VkFormat fmt, VkImageUsageFlags usage, VkImageCreateFlags flags,
        VkImageViewType view_type, VkImageAspectFlags view_aspects,
        VkImageLayout layout) {
    this->mip_levels = mip_lvls;
    return dev->create_img(
            dev_mem, extent, mip_levels, n, fmt, VK_IMAGE_TILING_OPTIMAL,
            usage, VK_SAMPLE_COUNT_1_BIT, flags,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            &this->img)
        && this->img.transition_layout(
            dev->id, queue, cmd_pool, view_aspects, VK_IMAGE_LAYOUT_UNDEFINED,
            layout, mip_levels, 0, n)
        && dev->create_img_view(
            this->img.id, fmt, view_type, view_aspects, mip_levels, 0, n,
            &this->img_view)
        && (view_type != VK_IMAGE_VIEW_TYPE_CUBE_ARRAY
            || dev->create_img_view(
                this->img.id, fmt, VK_IMAGE_VIEW_TYPE_2D_ARRAY,
                view_aspects, mip_levels, 0, n,
                &this->cube_2d_img_view));
}

bool TexArray::init_mipmaps(
        VkDevice dev, VkQueue queue, const CommandPool &cmd_pool,
        VkExtent2D extent, uint32_t base_layer, uint32_t n_layers) {
    return cmd_pool.with_cmd_buffer(
        dev, queue, [this, extent, base_layer, n_layers](auto b) {
            VkImageMemoryBarrier barrier = {};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.image = this->img.id;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseArrayLayer = base_layer;
            barrier.subresourceRange.layerCount = n_layers;
            barrier.subresourceRange.levelCount = 1;
            uint32_t mip_width = extent.width, mip_height = extent.height;
            for(uint32_t i = 1; i < mip_levels; ++i) {
                barrier.subresourceRange.baseMipLevel = i - 1;
                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                vkCmdPipelineBarrier(b,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    0, 0, nullptr, 0, nullptr, 1, &barrier);
                VkImageBlit blit = {};
                blit.srcOffsets[0] = {0, 0, 0};
                blit.srcOffsets[1] = {
                    static_cast<int32_t>(mip_width),
                    static_cast<int32_t>(mip_height), 1};
                blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                blit.srcSubresource.mipLevel = i - 1;
                blit.srcSubresource.baseArrayLayer = base_layer;
                blit.srcSubresource.layerCount = n_layers;
                blit.dstOffsets[0] = {0, 0, 0};
                blit.dstOffsets[1] = {
                    mip_width  ? static_cast<int32_t>(mip_width  / 2) : 1,
                    mip_height ? static_cast<int32_t>(mip_height / 2) : 1, 1};
                blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                blit.dstSubresource.mipLevel = i;
                blit.dstSubresource.baseArrayLayer = base_layer;
                blit.dstSubresource.layerCount = n_layers;
                vkCmdBlitImage(b,
                    this->img.id, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    this->img.id, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    1, &blit, VK_FILTER_NEAREST);
                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                vkCmdPipelineBarrier(b,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    0, 0, nullptr, 0, nullptr, 1, &barrier);
                if(mip_width) mip_width /= 2;
                if(mip_height) mip_height /= 2;
            }
            barrier.subresourceRange.baseMipLevel = mip_levels - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            vkCmdPipelineBarrier(b,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                0, 0, nullptr, 0, nullptr, 1, &barrier);
            return true;
        });
}

void TexArray::destroy(DeviceMemory *dev_mem, VkDevice dev) {
    for(auto &x : this->layer_views)
        nngn::destroy(&x, vkDestroyImageView, dev);
    this->layer_views.clear();
    for(auto *x : {&this->img_view, &this->cube_2d_img_view})
        nngn::destroy(x, vkDestroyImageView, dev);
    this->img.destroy(dev_mem, dev);
    this->mip_levels = 0;
}

bool TexArray::init_layer_views(
        Device *dev, uint32_t n, VkFormat fmt,
        VkImageViewType view_type, VkImageAspectFlags view_aspects) {
    auto &v = this->layer_views;
    v.clear();
    v.resize(n, VK_NULL_HANDLE);
    for(uint32_t i = 0; i < n; ++i)
        if(!dev->create_img_view(
                this->img.id, fmt, view_type, view_aspects, 1, i, 1, &v[i]))
            return false;
    return true;
}

bool ShadowMap::init(
        Device *dev, DeviceMemory *dev_mem,
        VkQueue queue, const CommandPool &cmd_pool, VkExtent2D extent) {
    this->destroy(dev_mem, dev->id);
    return TexArray::init(
            dev, dev_mem, queue, cmd_pool, extent, 1,
            nngn::Lighting::MAX_LIGHTS, DEPTH_FORMAT,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT
                | VK_IMAGE_USAGE_SAMPLED_BIT
                | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            0, VK_IMAGE_VIEW_TYPE_2D_ARRAY, VK_IMAGE_ASPECT_DEPTH_BIT,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL)
        && this->init_layer_views(
            dev, nngn::Lighting::MAX_LIGHTS, DEPTH_FORMAT,
            VK_IMAGE_VIEW_TYPE_2D_ARRAY, VK_IMAGE_ASPECT_DEPTH_BIT);
}

bool ShadowCube::init(
        Device *dev, DeviceMemory *dev_mem,
        VkQueue queue, const CommandPool &cmd_pool, VkExtent2D extent) {
    this->destroy(dev_mem, dev->id);
    return TexArray::init(
            dev, dev_mem, queue, cmd_pool, extent, 1,
            6 * nngn::Lighting::MAX_LIGHTS, DEPTH_FORMAT,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT
                | VK_IMAGE_USAGE_SAMPLED_BIT
                | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
            VK_IMAGE_VIEW_TYPE_CUBE_ARRAY, VK_IMAGE_ASPECT_DEPTH_BIT,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL)
        && this->init_layer_views(
            dev, 6 * nngn::Lighting::MAX_LIGHTS, DEPTH_FORMAT,
            VK_IMAGE_VIEW_TYPE_2D_ARRAY, VK_IMAGE_ASPECT_DEPTH_BIT);
}

VulkanBackend::VulkanBackend(const VulkanParameters &p) :
    GLFWBackend(p),
    log_level(p.log_level) {}

VulkanBackend::~VulkanBackend() {
    NNGN_LOG_CONTEXT_CF(VulkanBackend);
    if(!this->dev.id)
        return;
    vkDeviceWaitIdle(this->dev.id);
    vkFreeCommandBuffers(
        this->dev.id, this->dev.cmd_pool.id,
        static_cast<uint32_t>(this->cmd_buffers.size()),
        this->cmd_buffers.data());
    for(auto *x : {&this->tex_sampler, &this->shadow_sampler})
        if(*x)
            nngn::destroy(x, vkDestroySampler, this->dev.id);
    for(auto *x : {&this->tex, &this->font_tex})
        x->destroy(&this->dev_mem, this->dev.id);
    this->shadow_map.destroy(&this->dev_mem, this->dev.id);
    this->shadow_cube.destroy(&this->dev_mem, this->dev.id);
    for(const auto &v : {
            &this->camera_ubos, &this->camera_hud_ubos,
            &this->camera_shadow_ubos, &this->lights_ubos})
        for(auto &x : *v)
            x.destroy(&this->dev_mem, this->dev.id);
    for(auto *x : {
            &this->triangle_vbo, &this->triangle_ebo,
            &this->sprite_vbo, &this->sprite_ebo,
            &this->translucent_vbo, &this->translucent_ebo,
            &this->box_vbo, &this->box_ebo,
            &this->cube_vbo, &this->cube_ebo,
            &this->cube_dbg_vbo, &this->cube_dbg_ebo,
            &this->sphere_vbo, &this->sphere_ebo,
            &this->sphere_dbg_vbo, &this->sphere_dbg_ebo,
            &this->text_vbo, &this->text_ebo,
            &this->textbox_vbo, &this->textbox_ebo,
            &this->sel_vbo, &this->sel_ebo,
            &this->grid_vbo, &this->grid_ebo,
            &this->aabb_vbo, &this->aabb_ebo,
            &this->aabb_circle_vbo, &this->aabb_circle_ebo,
            &this->bb_vbo, &this->bb_ebo,
            &this->bb_circle_vbo, &this->bb_circle_ebo,
            &this->sphere_coll_vbo, &this->sphere_coll_ebo,
            &this->lights_vbo, &this->lights_ebo,
            &this->range_vbo, &this->range_ebo,
            &this->depth_vbo, &this->depth_ebo,
            &this->depth_cube_vbo, &this->depth_cube_ebo,
            &this->map_vbo, &this->map_ebo})
        x->destroy(&this->dev_mem, this->dev.id);
    this->dev.destroy(&this->dev_mem);
    this->dev_mem.destroy();
    this->instance.destroy();
}

std::tuple<int, int, int, std::string> VulkanBackend::version() const {
    uint32_t v = 0;
    if(!check_result(
            "vkEnumerateInstanceVersion", vkEnumerateInstanceVersion(&v))) {
        return {0, 0, 0, "error"};
    }
    return {
        static_cast<int>(VK_VERSION_MAJOR(v)),
        static_cast<int>(VK_VERSION_MINOR(v)),
        static_cast<int>(VK_VERSION_PATCH(v)), ""};
}

bool VulkanBackend::init() {
    NNGN_LOG_CONTEXT_CF(VulkanBackend);
    if(!GLFWBackend::init())
        return false;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    bool ok = true;
    ok = ok && this->create_window();
    ok = ok && (this->params.flags.is_set(Parameters::Flag::DEBUG)
        ? this->instance.create_debug(this->w, this->log_level)
        : this->instance.create(this->w, this->log_level));
    ok = ok && this->find_device();
    ok = ok && this->physical_dev.create_device(
        this->instance, VulkanBackend::graphics_family, &this->dev);
    ok = ok && (this->dev_mem.init(this->physical_dev.id), true);
    ok = ok && this->dev.create_sampler(
        VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, {},
        VK_SAMPLER_MIPMAP_MODE_NEAREST, Graphics::TEXTURE_MIP_LEVELS,
        &this->tex_sampler);
    ok = ok && this->instance.set_obj_name(
        this->dev.id, this->tex_sampler, "tex_sampler"sv);
    ok = ok && this->dev.create_sampler(
        VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE, VK_SAMPLER_MIPMAP_MODE_NEAREST,
        1, &this->shadow_sampler);
    ok = ok && this->instance.set_obj_name(
        this->dev.id, this->shadow_sampler, "shadow_sampler"sv);
    ok = ok && this->set_shadow_map_size(this->shadow_map_size);
    ok = ok && this->name_tex_array("shadow_map"sv, this->shadow_map);
    ok = ok && this->set_shadow_cube_size(this->shadow_cube_size);
    ok = ok && this->name_tex_array("shadow_cube"sv, this->shadow_cube);
    ok = ok && this->create_swapchain();
    ok = ok && this->dev.swapchain.init(
        this->instance, &this->dev_mem, &this->dev,
        this->physical_dev.surface_cap.currentExtent,
        {this->shadow_map_size, this->shadow_map_size},
        {this->shadow_cube_size, this->shadow_cube_size},
        this->shadow_map, this->shadow_cube);
    const auto n_imgs =
        static_cast<uint32_t>(this->dev.swapchain.img_views.size());
    ok = ok && this->create_uniform_buffer("camera_ubo"sv,
        n_imgs, sizeof(CameraUBO), &this->camera_ubos);
    ok = ok && this->create_uniform_buffer("camera_hud_ubos"sv,
        n_imgs, sizeof(CameraUBO), &this->camera_hud_ubos);
    ok = ok && this->create_uniform_buffer("camera_shadow_ubos"sv,
        7 * nngn::Lighting::MAX_LIGHTS, sizeof(CameraUBO),
        &this->camera_shadow_ubos);
    ok = ok && this->create_uniform_buffer("lights_ubos"sv,
        1 + n_imgs, sizeof(LightsUBO), &this->lights_ubos);
    ok = ok && this->dev.swapchain.camera_descriptor_sets.write(
        this->dev.id, n_imgs, this->camera_ubos.data(),
        this->camera_hud_ubos.data(), this->camera_shadow_ubos.data());
    ok = ok && this->dev.swapchain.texture_descriptor_sets.write(
        this->dev.id, this->tex_sampler, this->shadow_sampler,
        this->tex.img_view, this->font_tex.img_view,
        this->shadow_map.img_view, this->shadow_cube.cube_2d_img_view);
    ok = ok && this->dev.swapchain.lighting_descriptor_sets.write(
        this->dev.id, n_imgs, this->lights_ubos.data(),
        this->shadow_sampler,
        this->shadow_map.img_view, this->shadow_cube.img_view);
    if(!ok)
        return false;
    this->lights_ubos[0].memcpy(
        this->dev.id, &nngn::Lighting::NO_LIGHTS_UBO,
        sizeof(nngn::Lighting::NO_LIGHTS_UBO));
    glfwShowWindow(this->w);
    return true;
}

void VulkanBackend::resize(int, int) {
    this->flags |= Flag::RECREATE_SWAPCHAIN;
    this->set_camera_updated();
}

bool VulkanBackend::find_device() {
    NNGN_LOG_CONTEXT_CF(VulkanBackend);
    const auto check_graphics = [](auto d) {
        const auto v = enumerate<VkQueueFamilyProperties>(
            vkGetPhysicalDeviceQueueFamilyProperties, d);
        constexpr auto f = VulkanBackend::graphics_family;
        assert(f < v.size());
        if(f < v.size() && v[f].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            return true;
        Log::l() << "queue doesn't support graphics" << std::endl;
        return false;
    };
    const auto check_present = [this](auto d) {
        VkBool32 b = {};
        CHECK_RESULT(vkGetPhysicalDeviceSurfaceSupportKHR,
            d, this->graphics_family, this->instance.surface, &b);
        if(b)
            return true;
        Log::l() << "queue doesn't support present" << std::endl;
        return false;
    };
    const auto check_swapchain = [](auto d) {
        const auto v = enumerate<VkExtensionProperties>(
            vkEnumerateDeviceExtensionProperties, d, nullptr);
        const auto it = std::find_if(v.cbegin(), v.cend(), [](const auto &x)
            { return !std::strcmp(x.extensionName, SWAPCHAIN_EXTENSION); });
        if(it != v.cend())
            return true;
        Log::l() << "device doesn't support swap chain" << std::endl;
        return false;
    };
    const auto check_validations = [this](auto d) {
        if(!this->params.flags.is_set(Parameters::Flag::DEBUG))
            return true;
        const auto v = enumerate<VkLayerProperties>(
            vkEnumerateDeviceLayerProperties, d);
        for(const auto *const l : VALIDATION_LAYERS) {
            const auto it = std::find_if(
                v.cbegin(), v.cend(), [l](const auto &x)
                    { return !std::strcmp(x.layerName, l); });
            if(it != v.cend())
                return true;
            Log::l()
                << "device doesn't support validation layer "
                << std::quoted(l) << std::endl;
        }
        return false;
    };
    const auto check_format_supported = [this](auto d) {
        const auto v = enumerate<VkSurfaceFormatKHR>(
            vkGetPhysicalDeviceSurfaceFormatsKHR, d, this->instance.surface);
        const auto it = std::find_if(
            v.cbegin(), v.cend(), [](const auto &x) {
                return x.format == SURFACE_FORMAT
                    && x.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR; });
        if(it != v.cend())
            return true;
        Log::l() << "surface doesn't support format" << std::endl;
        return false;
    };
    const auto check_present_supported = [this](auto d) {
        const auto v = enumerate<VkPresentModeKHR>(
            vkGetPhysicalDeviceSurfacePresentModesKHR,
            d, this->instance.surface);
        const auto it = std::find(
            v.cbegin(), v.cend(), VK_PRESENT_MODE_FIFO_KHR);
        if(it != v.cend())
            return true;
        Log::l() << "surface doesn't support present mode" << std::endl;
        return false;
    };
    const auto check_features = [](auto d) {
        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(d, &features);
        if(!features.fillModeNonSolid) {
            Log::l()
                << "device doesn't support VK_POLYGON_MODE_LINE"
                << std::endl;
            return false;
        }
        if(!features.imageCubeArray) {
            Log::l()
                << "device doesn't support VK_IMAGE_VIEW_TYPE_CUBE_ARRAY"
                << std::endl;
            return false;
        }
        return true;
    };
    const auto v = enumerate<VkPhysicalDevice>(
        vkEnumeratePhysicalDevices, this->instance.id);
    for(auto *const x : v) {
        bool ok = true;
        ok = ok && check_graphics(x);
        ok = ok && check_present(x);
        ok = ok && check_swapchain(x);
        ok = ok && check_validations(x);
        ok = ok && check_format_supported(x);
        ok = ok && check_present_supported(x);
        ok = ok && check_features(x);
        if(!ok)
            continue;
        this->physical_dev.id = x;
        this->physical_dev.update_surface_cap(this->instance.surface);
        return true;
    }
    Log::l() << "no devices found" << std::endl;
    return false;
}

bool VulkanBackend::create_swapchain() {
    NNGN_LOG_CONTEXT_CF(VulkanBackend);
    VkSwapchainCreateInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    info.surface = this->instance.surface;
    info.minImageCount = this->physical_dev.surface_cap.minImageCount;
    info.imageFormat = SURFACE_FORMAT;
    info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    info.imageExtent = this->physical_dev.surface_cap.currentExtent;
    info.imageArrayLayers = 1;
    info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if(this->dev.graphics_queue == this->dev.present_queue)
        info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    else {
        std::array<uint32_t, 2> v =
            {VulkanBackend::graphics_family, VulkanBackend::present_family};
        info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        info.queueFamilyIndexCount = v.size();
        info.pQueueFamilyIndices = v.data();
    }
    info.preTransform = this->physical_dev.surface_cap.currentTransform;
    info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    info.presentMode = this->present_mode;
    info.clipped = VK_TRUE;
    info.oldSwapchain = VK_NULL_HANDLE;
    CHECK_RESULT(vkCreateSwapchainKHR,
        this->dev.id, &info, nullptr, &this->dev.swapchain.id);
    return true;
}

bool VulkanBackend::recreate_swapchain() {
    NNGN_LOG_CONTEXT_CF(VulkanBackend);
    this->flags.clear(Flag::RECREATE_SWAPCHAIN);
    vkDeviceWaitIdle(this->dev.id);
    this->dev.swapchain.soft_destroy(&this->dev_mem, this->dev.id);
    this->physical_dev.update_surface_cap(this->instance.surface);
    return this->create_swapchain()
        && this->dev.swapchain.recreate(
            this->instance, &this->dev_mem, &this->dev,
            this->physical_dev.surface_cap.currentExtent,
            {this->shadow_map_size, this->shadow_map_size},
            {this->shadow_cube_size, this->shadow_cube_size},
            this->shadow_map, this->shadow_cube)
        && this->create_cmd_buffers();
}

bool VulkanBackend::name_tex_array(
        std::string_view name, const TexArray &t) const {
    NNGN_LOG_CONTEXT_CF(VulkanBackend);
    bool ok = this->instance.set_obj_name(this->dev.id, t.img.id, name)
        && this->instance.set_obj_name(
            this->dev.id, t.img.mem, name.data() + "_mem"s)
        && this->instance.set_obj_name(
            this->dev.id, t.img_view, name.data() + "_view"s);
    if(!ok)
        return false;
    for(size_t i = 0, n = t.layer_views.size(); i < n; ++i)
        if(!this->instance.set_obj_name_n(
                this->dev.id, t.layer_views[i], name.data() + "_layer"s, i))
            return false;
    return true;
}

bool VulkanBackend::create_uniform_buffer(
        std::string_view name,
        size_t n, VkDeviceSize size, std::vector<Buffer> *ubos) {
    NNGN_LOG_CONTEXT_CF(VulkanBackend);
    ubos->resize(n);
    for(size_t i = 0; i < n; ++i) {
        auto &x = (*ubos)[i];
        if(!x.init(
                this->dev.id, &this->dev_mem,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                    | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                size))
            return false;
        const auto iname = name.data() + std::to_string(i);
        if(!this->instance.set_obj_name(this->dev.id, x.id, iname))
            return false;
        if(!this->instance.set_obj_name(this->dev.id, x.mem, iname + "_mem"))
            return false;
    }
    return true;
}

bool VulkanBackend::create_vertex_buffer(
        std::string_view name, VkDeviceSize size, Buffer *b) {
    NNGN_LOG_CONTEXT_CF(VulkanBackend);
    return b->init(
            this->dev.id, &this->dev_mem,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT
                | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, size)
        && this->instance.set_obj_name(this->dev.id, b->id, name)
        && this->instance.set_obj_name(
            this->dev.id, b->mem, name.data() + "_mem"s);
}

bool VulkanBackend::create_index_buffer(
        std::string_view name, VkDeviceSize size, Buffer *b) {
    NNGN_LOG_CONTEXT_CF(VulkanBackend);
    return b->init(
            this->dev.id, &this->dev_mem,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT
                | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, size)
        && this->instance.set_obj_name(this->dev.id, b->id, name)
        && this->instance.set_obj_name(
            this->dev.id, b->mem, name.data() + "_mem"s);
}

bool VulkanBackend::copy_buffer(
        VkBuffer dst, VkBuffer src, const VkBufferCopy &copy) const {
    NNGN_LOG_CONTEXT_CF(VulkanBackend);
    return this->dev.tmp_cmd_pool.with_cmd_buffer(
        this->dev.id, this->dev.graphics_queue,
        [dst, src, &copy](auto b)
            { vkCmdCopyBuffer(b, src, dst, 1, &copy); return true; });
}

bool VulkanBackend::copy_buffer(
        VkExtent2D extent, uint32_t base_layer, uint32_t n_layers,
        VkBuffer src, VkImage dst) const {
    NNGN_LOG_CONTEXT_CF(VulkanBackend);
    return this->dev.tmp_cmd_pool.with_cmd_buffer(
        this->dev.id, this->dev.graphics_queue,
        [extent, base_layer, n_layers, src, dst](auto b) {
            VkBufferImageCopy region = {};
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = base_layer;
            region.imageSubresource.layerCount = n_layers;
            region.imageOffset = {0, 0, 0};
            region.imageExtent = {extent.width, extent.height, 1};
            vkCmdCopyBufferToImage(
                b, src, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
            return true;
        });
}

bool VulkanBackend::create_cmd_buffers() {
    NNGN_LOG_CONTEXT_CF(VulkanBackend);
    struct RenderList {
        struct R {
            VkPipeline pipeline;
            std::vector<std::pair<Buffer*, Buffer*>> buffers;
        };
        std::vector<R>
            depth_dir, depth_point, map_ortho, map_persp, normal, no_light,
            overlay, hud, shadow_maps, shadow_cubes;
    } const render_list = {{
        {this->dev.swapchain.map_depth_pipeline[0], {
            {&this->map_vbo, &this->map_ebo}}},
        {this->dev.swapchain.triangle_depth_pipeline[0], {
            {&this->cube_vbo, &this->cube_ebo}}},
        {this->dev.swapchain.sprite_depth_pipeline[0], {
            {&this->sprite_vbo, &this->sprite_ebo}}},
    }, {
        {this->dev.swapchain.map_depth_pipeline[1], {
            {&this->map_vbo, &this->map_ebo}}},
        {this->dev.swapchain.triangle_depth_pipeline[1], {
            {&this->triangle_vbo, &this->triangle_ebo},
            {&this->cube_vbo, &this->cube_ebo}}},
        {this->dev.swapchain.sprite_depth_pipeline[1], {
            {&this->sprite_vbo, &this->sprite_ebo}}},
    }, {
        {this->dev.swapchain.map_ortho_pipeline, {
            {&this->map_vbo, &this->map_ebo}}},
    }, {
        {this->dev.swapchain.map_persp_pipeline, {
            {&this->map_vbo, &this->map_ebo}}},
    }, {
        {this->dev.swapchain.triangle_pipeline, {
            {&this->cube_vbo, &this->cube_ebo}}},
        {this->dev.swapchain.sprite_pipeline, {
            {&this->sprite_vbo, &this->sprite_ebo}}},
    }, {
        {this->dev.swapchain.sprite_pipeline, {
            {&this->translucent_vbo, &this->translucent_ebo}}},
    }, {
        {this->dev.swapchain.circle_pipeline, {
            {&this->aabb_circle_vbo, &this->aabb_circle_ebo},
            {&this->bb_circle_vbo, &this->bb_circle_ebo},
            {&this->sphere_coll_vbo, &this->sphere_coll_ebo}}},
        {this->dev.swapchain.box_pipeline, {
            {&this->box_vbo, &this->box_ebo},
            {&this->sel_vbo, &this->sel_ebo},
            {&this->aabb_vbo, &this->aabb_ebo},
            {&this->bb_vbo, &this->bb_ebo},
            {&this->cube_dbg_vbo, &this->cube_dbg_ebo},
            {&this->lights_vbo, &this->lights_ebo}}},
        {this->dev.swapchain.wire_pipeline, {
            {&this->range_vbo, &this->range_ebo}}},
        {this->dev.swapchain.grid_pipeline, {
            {&this->grid_vbo, &this->grid_ebo}}},
    }, {
        {this->dev.swapchain.box_pipeline, {
            {&this->textbox_vbo, &this->textbox_ebo}}},
        {this->dev.swapchain.font_pipeline, {
            {&this->text_vbo, &this->text_ebo}}},
    }, {
        {this->dev.swapchain.circle_pipeline, {
            {&this->depth_vbo, &this->depth_ebo}}},
    }, {
        {this->dev.swapchain.circle_pipeline, {
            {&this->depth_cube_vbo, &this->depth_cube_ebo}}},
        {this->dev.swapchain.box_pipeline, {
            {&this->triangle_vbo, &this->triangle_ebo}}},
    }};
    const auto bind_descriptors = [this](
            auto b, uint32_t off, uint32_t n, const auto *p) {
        vkCmdBindDescriptorSets(
            b, VK_PIPELINE_BIND_POINT_GRAPHICS,
            this->dev.swapchain.render_pipeline_layout, off, n, p, 0, nullptr);
    };
    const auto render = [](auto b, const auto &l) {
        for(auto &x : l) {
            vkCmdBindPipeline(b, VK_PIPELINE_BIND_POINT_GRAPHICS, x.pipeline);
            for(auto &v : x.buffers) {
                const auto &vbo = v.first, &ebo = v.second;
                if(!ebo->size)
                    continue;
                constexpr VkDeviceSize offset = 0;
                vkCmdBindVertexBuffers(b, 0, 1, &vbo->id, &offset);
                vkCmdBindIndexBuffer(b, ebo->id, 0, VK_INDEX_TYPE_UINT32);
                vkCmdDrawIndexed(b,
                    static_cast<uint32_t>(ebo->size / sizeof(uint32_t)),
                    1, 0, 0, 0);
            }
        }
    };
    const auto create_depth_pass =
            [this, bind_descriptors, render, &render_list]
            (auto b, size_t il) {
        const bool is_dir = il < nngn::Lighting::MAX_LIGHTS;
        VkClearValue clear_value = {};
        clear_value.depthStencil = {1, 0};
        VkRenderPassBeginInfo pass_info = {};
        pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        pass_info.renderPass = this->dev.swapchain.depth_pass;
        pass_info.framebuffer = this->dev.swapchain.depth_framebuffers[il];
        pass_info.renderArea.offset = {0, 0};
        pass_info.renderArea.extent = is_dir
            ? VkExtent2D{shadow_map_size, shadow_map_size}
            : VkExtent2D{shadow_cube_size, shadow_cube_size};
        pass_info.clearValueCount = 1;
        pass_info.pClearValues = &clear_value;
        vkCmdBeginRenderPass(b, &pass_info, VK_SUBPASS_CONTENTS_INLINE);
        bind_descriptors(b, 0, 3, std::array{
            this->dev.swapchain.lighting_descriptor_sets.ids[0],
            this->dev.swapchain.camera_descriptor_sets.ids[
                this->dev.swapchain.img_views.size() * 2 + il],
            this->dev.swapchain.texture_descriptor_sets.ids[0]}.data());
        render(b, is_dir ? render_list.depth_dir : render_list.depth_point);
        vkCmdEndRenderPass(b);
        return true;
    };
    const auto create_render_pass =
            [this, bind_descriptors, render, &render_list]
            (auto b, size_t i) {
        std::array<VkClearValue, 2> clear_values = {};
        clear_values[0].color = {{0, 0, 0, 1}};
        clear_values[1].depthStencil = {1, 0};
        VkRenderPassBeginInfo pass_info = {};
        pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        pass_info.renderPass = this->dev.swapchain.render_pass;
        pass_info.framebuffer = this->dev.swapchain.framebuffers[i];
        pass_info.renderArea.offset = {0, 0};
        pass_info.renderArea.extent =
            this->physical_dev.surface_cap.currentExtent;
        pass_info.clearValueCount = clear_values.size();
        pass_info.pClearValues = clear_values.data();
        const auto push_alpha = [this, b](float a) {
            vkCmdPushConstants(
                b, this->dev.swapchain.render_pipeline_layout,
                VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float), &a);
        };
        vkCmdBeginRenderPass(b, &pass_info, VK_SUBPASS_CONTENTS_INLINE);
        bind_descriptors(b, 0, 3, std::array{
            this->dev.swapchain.lighting_descriptor_sets.ids[i + 1],
            this->dev.swapchain.camera_descriptor_sets.ids[i],
            this->dev.swapchain.texture_descriptor_sets.ids[0]}.data());
        push_alpha(1);
        render(b,
            *this->camera.flags & nngn::Camera::Flag::PERSPECTIVE
            ? render_list.map_persp : render_list.map_ortho);
        render(b, render_list.normal);
        bind_descriptors(b, 0, 1,
            &this->dev.swapchain.lighting_descriptor_sets.ids[0]);
        render(b, render_list.no_light);
        push_alpha(.5);
        render(b, render_list.overlay);
        bind_descriptors(b, 1, 2, std::array{
            this->dev.swapchain.camera_descriptor_sets.ids[
                this->dev.swapchain.img_views.size() + i],
            this->dev.swapchain.texture_descriptor_sets.ids[1]}.data());
        render(b, render_list.hud);
        bind_descriptors(b, 2, 1,
            &this->dev.swapchain.texture_descriptor_sets.ids[2]);
        render(b, render_list.shadow_maps);
        bind_descriptors(b, 2, 1,
            &this->dev.swapchain.texture_descriptor_sets.ids[3]);
        render(b, render_list.shadow_cubes);
        vkCmdEndRenderPass(b);
        return true;
    };
    const auto n = this->dev.swapchain.img_views.size();
    auto &v = this->cmd_buffers;
    vkFreeCommandBuffers(
        this->dev.id, this->dev.cmd_pool.id,
        static_cast<uint32_t>(v.size()), v.data());
    v.resize(n);
    VkCommandBufferAllocateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.commandPool = this->dev.cmd_pool.id;
    info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    info.commandBufferCount = static_cast<uint32_t>(n);
    CHECK_RESULT(vkAllocateCommandBuffers, this->dev.id, &info, v.data());
    for(size_t i = 0; i < n; ++i) {
        auto *const b = this->cmd_buffers[i];
        if(!this->instance.set_obj_name_n(
                this->dev.id, b, "render_pass_cmd_buffer"sv, i))
            return false;
        VkCommandBufferBeginInfo begin_info = {};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        CHECK_RESULT(vkBeginCommandBuffer, b, &begin_info);
        const auto &ubo = *this->lighting.ubo;
        for(size_t l = 0, n_dir = ubo.n_dir; l < n_dir; ++l)
            if(!create_depth_pass(b, l))
                return false;
        for(size_t l = 0, n_point = 6 * ubo.n_point; l < n_point; ++l)
            if(!create_depth_pass(b, nngn::Lighting::MAX_LIGHTS + l))
                return false;
        if(!create_render_pass(b, i))
            return false;
        CHECK_RESULT(vkEndCommandBuffer, b);
    }
    this->flags.clear(Flag::RECREATE_CMDS);
    return true;
}

bool VulkanBackend::set_shadow_map_size(uint32_t s) {
    NNGN_LOG_CONTEXT_CF(VulkanBackend);
    if(this->shadow_map.img.id)
        this->shadow_map.img.destroy(&this->dev_mem, this->dev.id);
    this->shadow_map_size = s;
    bool ok = true;
    ok = ok && this->shadow_map.init(
        &this->dev, &this->dev_mem,
        this->dev.graphics_queue, this->dev.tmp_cmd_pool, {s, s});
    if(this->dev.swapchain.id) {
        ok = ok && this->dev.swapchain.texture_descriptor_sets.write(
            this->dev.id, this->tex_sampler, this->shadow_sampler,
            this->tex.img_view, this->font_tex.img_view,
            this->shadow_map.img_view, this->shadow_cube.cube_2d_img_view);
        ok = ok && this->dev.swapchain.lighting_descriptor_sets.write(
            this->dev.id,
            static_cast<uint32_t>(this->dev.swapchain.img_views.size()),
            this->lights_ubos.data(), this->shadow_sampler,
            this->shadow_map.img_view, this->shadow_cube.img_view);
    }
    this->flags |= Flag::RECREATE_SWAPCHAIN | Flag::RECREATE_CMDS;
    return ok;
}

bool VulkanBackend::set_shadow_cube_size(uint32_t s) {
    NNGN_LOG_CONTEXT_CF(VulkanBackend);
    if(this->shadow_cube.img.id)
        this->shadow_cube.img.destroy(&this->dev_mem, this->dev.id);
    this->shadow_cube_size = s;
    bool ok = true;
    ok = ok && this->shadow_cube.init(
        &this->dev, &this->dev_mem,
        this->dev.graphics_queue, this->dev.tmp_cmd_pool, {s, s});
    if(this->dev.swapchain.id) {
        ok = ok && this->dev.swapchain.texture_descriptor_sets.write(
            this->dev.id, this->tex_sampler, this->shadow_sampler,
            this->tex.img_view, this->font_tex.img_view,
            this->shadow_map.img_view, this->shadow_cube.cube_2d_img_view);
        ok = ok && this->dev.swapchain.lighting_descriptor_sets.write(
            this->dev.id,
            static_cast<uint32_t>(this->dev.swapchain.img_views.size()),
            this->lights_ubos.data(), this->shadow_sampler,
            this->shadow_map.img_view, this->shadow_cube.img_view);
    }
    this->flags |= Flag::RECREATE_SWAPCHAIN | Flag::RECREATE_CMDS;
    return ok;
}

bool VulkanBackend::create_stg_buffer(uint64_t size, StagingBuffer *stg) {
    NNGN_LOG_CONTEXT_CF(VulkanBackend);
    this->stg_buffers.emplace_front();
    auto *b = &this->stg_buffers.front();
    if(!b->init(
            this->dev.id, &this->dev_mem,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            size))
        return false;
    if(!this->instance.set_obj_name(this->dev.id, b->id, "stg_buffer"sv))
        return false;
    if(!this->instance.set_obj_name(this->dev.id, b->mem, "stg_buffer_mem"sv))
        return false;
    void *p = {};
    vkMapMemory(this->dev.id, b->mem, 0, size, 0, &p);
    stg->g = this;
    stg->b = b;
    stg->p = static_cast<std::byte*>(static_cast<void*>(p));
    return true;
}

bool VulkanBackend::create_stg_buffer_pair(
        uint64_t vsize, uint64_t isize,
        std::pair<StagingBuffer, StagingBuffer> *stg) {
    NNGN_LOG_CONTEXT_CF(VulkanBackend);
    return this->create_stg_buffer(vsize + isize, &stg->first);
}

bool VulkanBackend::unmap_stg_buffer(StagingBuffer *stg) {
    NNGN_LOG_CONTEXT_CF(VulkanBackend);
    if(!stg->p) {
        vkUnmapMemory(this->dev.id, static_cast<Buffer*>(stg->b)->mem);
        stg->p = nullptr;
    }
    return true;
}

Buffer *VulkanBackend::buffer_target(TargetBuffer b) {
    switch(b) {
    case TargetBuffer::TRIANGLE_VBO: return &this->triangle_vbo;
    case TargetBuffer::TRIANGLE_EBO: return &this->triangle_ebo;
    case TargetBuffer::SPRITE_VBO: return &this->sprite_vbo;
    case TargetBuffer::SPRITE_EBO: return &this->sprite_ebo;
    case TargetBuffer::TRANSLUCENT_VBO: return &this->translucent_vbo;
    case TargetBuffer::TRANSLUCENT_EBO: return &this->translucent_ebo;
    case TargetBuffer::BOX_VBO: return &this->box_vbo;
    case TargetBuffer::BOX_EBO: return &this->box_ebo;
    case TargetBuffer::SELECTION_VBO: return &this->sel_vbo;
    case TargetBuffer::SELECTION_EBO: return &this->sel_ebo;
    case TargetBuffer::AABB_VBO: return &this->aabb_vbo;
    case TargetBuffer::AABB_EBO: return &this->aabb_ebo;
    case TargetBuffer::AABB_CIRCLE_VBO: return &this->aabb_circle_vbo;
    case TargetBuffer::AABB_CIRCLE_EBO: return &this->aabb_circle_ebo;
    case TargetBuffer::BB_VBO: return &this->bb_vbo;
    case TargetBuffer::BB_EBO: return &this->bb_ebo;
    case TargetBuffer::BB_CIRCLE_VBO: return &this->bb_circle_vbo;
    case TargetBuffer::BB_CIRCLE_EBO: return &this->bb_circle_ebo;
    case TargetBuffer::SPHERE_COLL_VBO: return &this->sphere_coll_vbo;
    case TargetBuffer::SPHERE_COLL_EBO: return &this->sphere_coll_ebo;
    case TargetBuffer::CUBE_VBO: return &this->cube_vbo;
    case TargetBuffer::CUBE_EBO: return &this->cube_ebo;
    case TargetBuffer::CUBE_DEBUG_VBO: return &this->cube_dbg_vbo;
    case TargetBuffer::CUBE_DEBUG_EBO: return &this->cube_dbg_ebo;
    case TargetBuffer::SPHERE_VBO: return &this->sphere_vbo;
    case TargetBuffer::SPHERE_EBO: return &this->sphere_ebo;
    case TargetBuffer::SPHERE_DEBUG_VBO: return &this->sphere_dbg_vbo;
    case TargetBuffer::SPHERE_DEBUG_EBO: return &this->sphere_dbg_ebo;
    case TargetBuffer::TEXT_VBO: return &this->text_vbo;
    case TargetBuffer::TEXT_EBO: return &this->text_ebo;
    case TargetBuffer::TEXTBOX_VBO: return &this->textbox_vbo;
    case TargetBuffer::TEXTBOX_EBO: return &this->textbox_ebo;
    case TargetBuffer::GRID_VBO: return &this->grid_vbo;
    case TargetBuffer::GRID_EBO: return &this->grid_ebo;
    case TargetBuffer::LIGHTS_VBO: return &this->lights_vbo;
    case TargetBuffer::LIGHTS_EBO: return &this->lights_ebo;
    case TargetBuffer::RANGE_VBO: return &this->range_vbo;
    case TargetBuffer::RANGE_EBO: return &this->range_ebo;
    case TargetBuffer::DEPTH_VBO: return &this->depth_vbo;
    case TargetBuffer::DEPTH_EBO: return &this->depth_ebo;
    case TargetBuffer::DEPTH_CUBE_VBO: return &this->depth_cube_vbo;
    case TargetBuffer::DEPTH_CUBE_EBO: return &this->depth_cube_ebo;
    case TargetBuffer::MAP_VBO: return &this->map_vbo;
    case TargetBuffer::MAP_EBO: return &this->map_ebo;
    case TargetBuffer::MAX:
    default: break;
    }
    Log::l()
        << "invalid buffer: "
        << static_cast<std::underlying_type_t<TargetBuffer>>(b)
        << std::endl;
    return nullptr;
}

bool VulkanBackend::copy_to_buffer(
        VkBuffer b, const StagingBuffer &stg,
        uint64_t offset, uint64_t length) {
    NNGN_LOG_CONTEXT_CF(VulkanBackend);
    return this->copy_buffer(
        b, static_cast<Buffer*>(stg.b)->id, {offset, 0, length});
}

bool VulkanBackend::copy_to_buffer(
        TargetBuffer b, const StagingBuffer &stg,
        uint64_t offset, uint64_t length) {
    NNGN_LOG_CONTEXT_CF(VulkanBackend);
    auto *tb = this->buffer_target(b);
    return tb && this->copy_to_buffer(tb->id, stg, offset, length);
}

bool VulkanBackend::destroy_stg_buffer(StagingBuffer *stg) {
    NNGN_LOG_CONTEXT_CF(VulkanBackend);
    auto *b = static_cast<Buffer*>(stg->b);
    const auto it = std::find_if(
        this->stg_buffers.begin(), this->stg_buffers.end(),
        [id = b->id](const auto &x) { return x.id == id; });
    assert(it != this->stg_buffers.end());
    if(!this->unmap_stg_buffer(stg))
        return false;
    b->destroy(&this->dev_mem, this->dev.id);
    this->stg_buffers.erase(it);
    return true;
}

bool VulkanBackend::set_buffer_pair_capacity(
        TargetBuffer vbo, uint64_t vcap,
        TargetBuffer ebo, uint64_t ecap) {
    NNGN_LOG_CONTEXT_CF(VulkanBackend);
    auto *const vbt = this->buffer_target(vbo);
    auto *const ebt = this->buffer_target(ebo);
    if(!vbt || !ebt)
        return false;
    assert(!vbt->id);
    assert(!ebt->id);
    return this->create_vertex_buffer(
            Graphics::TARGET_BUFFER_NAME[static_cast<size_t>(vbo)],
            vcap, vbt)
        && this->create_index_buffer(
            Graphics::TARGET_BUFFER_NAME[static_cast<size_t>(ebo)],
            ecap, ebt);
}

bool VulkanBackend::set_buffer_size(TargetBuffer b, uint64_t size) {
    NNGN_LOG_CONTEXT_CF(VulkanBackend);
    auto *tb = this->buffer_target(b);
    if(!tb)
        return false;
    if(tb->size == size)
        return true;
    assert(size <= tb->capacity);
    tb->size = size;
    this->flags |= Flag::RECREATE_CMDS;
    return true;
}

void VulkanBackend::set_swap_interval(int i) {
    this->m_swap_interval = i;
    auto p = i ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR;
    if(p != this->present_mode) {
        this->present_mode = p;
        this->flags |= Flag::RECREATE_SWAPCHAIN;
    }
}

bool VulkanBackend::resize_textures(uint32_t s) {
    this->tex.destroy(&this->dev_mem, this->dev.id);
    return this->tex.init(
            &this->dev, &this->dev_mem,
            this->dev.graphics_queue, this->dev.tmp_cmd_pool,
            {Graphics::TEXTURE_EXTENT, Graphics::TEXTURE_EXTENT},
            Graphics::TEXTURE_MIP_LEVELS, s,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT
                | VK_IMAGE_USAGE_TRANSFER_DST_BIT
                | VK_IMAGE_USAGE_SAMPLED_BIT, 0,
            VK_IMAGE_VIEW_TYPE_2D_ARRAY, VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        && this->name_tex_array("tex"sv, this->tex)
        && this->dev.swapchain.texture_descriptor_sets.write(
            this->dev.id, this->tex_sampler, this->shadow_sampler,
            this->tex.img_view, this->font_tex.img_view,
            this->shadow_map.img_view, this->shadow_cube.cube_2d_img_view)
        && (this->flags |= Flag::RECREATE_CMDS);
}

bool VulkanBackend::load_textures(uint32_t i, uint32_t n, const std::byte *v) {
    NNGN_LOG_CONTEXT_CF(VulkanBackend);
    if(!this->tex.img.transition_layout(
            this->dev.id, this->dev.graphics_queue, this->dev.tmp_cmd_pool,
            VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            Graphics::TEXTURE_MIP_LEVELS, i, n))
        return false;
    const auto size = n * Graphics::TEXTURE_SIZE;
    StagingBuffer stg;
    if(!this->create_stg_buffer(size, &stg))
        return false;
    std::memcpy(stg.p, v, size);
    if(!this->unmap_stg_buffer(&stg))
        return false;
    constexpr VkExtent2D extent =
        {Graphics::TEXTURE_EXTENT, Graphics::TEXTURE_EXTENT};
    const auto buf_id = static_cast<Buffer*>(stg.b)->id;
    return this->copy_buffer(extent, i, n, buf_id, this->tex.img.id)
        && this->tex.init_mipmaps(
            this->dev.id, this->dev.graphics_queue, this->dev.tmp_cmd_pool,
            extent, i, n);
}

bool VulkanBackend::resize_font(uint32_t s) {
    NNGN_LOG_CONTEXT_CF(VulkanBackend);
    this->font_tex.destroy(&this->dev_mem, this->dev.id);
    return this->font_tex.init(
            &this->dev, &this->dev_mem,
            this->dev.graphics_queue, this->dev.tmp_cmd_pool,
            {s, s}, Graphics::mip_levels_for_extent(s), Font::N,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 0,
            VK_IMAGE_VIEW_TYPE_2D_ARRAY, VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        && this->name_tex_array("font_tex"sv, this->font_tex)
        && this->dev.swapchain.texture_descriptor_sets.write(
            this->dev.id, this->tex_sampler, this->shadow_sampler,
            this->tex.img_view, this->font_tex.img_view,
            this->shadow_map.img_view, this->shadow_cube.cube_2d_img_view)
        && (this->flags |= Flag::RECREATE_CMDS);
}

bool VulkanBackend::load_font(
        unsigned char c, uint32_t n,
        const nngn::uvec2 *size, const std::byte *v) {
    NNGN_LOG_CONTEXT_CF(VulkanBackend);
    if(!this->font_tex.img.transition_layout(
            this->dev.id, this->dev.graphics_queue, this->dev.tmp_cmd_pool,
            VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            this->font_tex.mip_levels, c, n))
        return false;
    size_t buf_size = 0;
    for(size_t i = 0; i < n; ++i)
        buf_size += size[i].x * size[i].y;
    buf_size *= 4;
    if(buf_size) {
        StagingBuffer stg;
        if(!this->create_stg_buffer(buf_size, &stg))
            return false;
        std::memcpy(stg.p, v, buf_size);
        if(!this->unmap_stg_buffer(&stg))
            return false;
        const auto buf_id = static_cast<Buffer*>(stg.b)->id;
        for(size_t i = 0; i < n; ++i)
            if(!this->copy_buffer(
                    {size[i].x, size[i].y}, static_cast<uint32_t>(c + i),
                    1, buf_id, this->font_tex.img.id))
                return false;
    }
    return this->font_tex.img.transition_layout(
        this->dev.id, this->dev.graphics_queue, this->dev.tmp_cmd_pool,
        VK_IMAGE_ASPECT_COLOR_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        this->font_tex.mip_levels, c, n);
}

bool VulkanBackend::render() {
    NNGN_LOG_CONTEXT_CF(VulkanBackend);
    auto prof = nngn::Profile::context<nngn::Profile>(
        &nngn::Profile::stats.render);
    const auto submit = [this](auto img_idx) {
        constexpr VkPipelineStageFlags stage =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        const auto &i = this->iframe;
        VkSubmitInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores = &this->dev.swapchain.image_available_sem[i];
        info.pWaitDstStageMask = &stage;
        info.commandBufferCount = 1;
        info.pCommandBuffers = &this->cmd_buffers[img_idx];
        info.signalSemaphoreCount = 1;
        info.pSignalSemaphores = &this->dev.swapchain.render_finished_sem[i];
        CHECK_RESULT(vkQueueSubmit,
            this->dev.graphics_queue, 1, &info, this->dev.swapchain.fences[i]);
        return true;
    };
    const auto present = [this](auto img_idx) {
        VkPresentInfoKHR info = {};
        info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores =
            &this->dev.swapchain.render_finished_sem[this->iframe];
        info.swapchainCount = 1;
        info.pSwapchains = &this->dev.swapchain.id;
        info.pImageIndices = &img_idx;
        return vkQueuePresentKHR(this->dev.present_queue, &info);
    };
    const auto update_camera = [this](auto img_idx) {
        if(!(this->camera_updated & (1u << img_idx)))
            return;
        this->camera_updated = static_cast<uint8_t>(
            this->camera_updated & ~(1u << img_idx));
        CameraUBO c = {CLIP_PROJ * *this->camera.proj, *this->camera.view};
        this->camera_ubos[img_idx].memcpy(this->dev.id, &c, sizeof(c));
        c = {CLIP_PROJ * *this->camera.hud_proj, nngn::mat4(1)};
        this->camera_hud_ubos[img_idx].memcpy(this->dev.id, &c, sizeof(c));
    };
    const auto update_lighting = [this](auto img_idx) {
        if(!(this->lighting_updated & (1u << img_idx)))
            return;
        this->lighting_updated = static_cast<uint8_t>(
            this->lighting_updated & ~(1u << img_idx));
        const auto &ubo = *this->lighting.ubo;
        this->lights_ubos[img_idx + 1].memcpy(this->dev.id, &ubo, sizeof(ubo));
        CameraUBO c = {};
        auto *dst = this->camera_shadow_ubos.data();
        const auto update_ubo = [this, &c, &dst](const auto &view) {
            c.view = view;
            (dst++)->memcpy(this->dev.id, &c, sizeof(CameraUBO));
        };
        c.proj = CLIP_PROJ * *this->lighting.dir_proj;
        const auto *dir_views = this->lighting.dir_views;
        for(size_t i = 0, n = ubo.n_dir; i < n; ++i)
            update_ubo(*dir_views++);
        dst = this->camera_shadow_ubos.data() + nngn::Lighting::MAX_LIGHTS;
        c.proj = CLIP_PROJ * *this->lighting.point_proj;
        const auto *point_views = this->lighting.point_views;
        for(size_t i = 0, n = ubo.n_point; i < n; ++i)
            for(size_t f = 0; f < 6; ++f)
                update_ubo(*point_views++);
        this->flags |= Flag::RECREATE_CMDS;
    };
    if(this->params.flags.is_set(Parameters::Flag::DEBUG))
        for(const auto &x : this->stg_buffers)
            Log::l() << "staging buffer leak: " << x.id << std::endl;
    if(this->flags & Flag::RECREATE_SWAPCHAIN && !this->recreate_swapchain())
        return false;
    if(this->flags & Flag::RECREATE_CMDS && !this->create_cmd_buffers())
        return false;
    vkWaitForFences(
        this->dev.id, 1, &this->dev.swapchain.fences[this->iframe],
        VK_TRUE, std::numeric_limits<uint64_t>::max());
    uint32_t img_idx = {};
    auto result = vkAcquireNextImageKHR(
        this->dev.id, this->dev.swapchain.id,
        std::numeric_limits<uint64_t>::max(),
        this->dev.swapchain.image_available_sem[this->iframe],
        VK_NULL_HANDLE, &img_idx);
    if(result == VK_ERROR_OUT_OF_DATE_KHR)
        return this->recreate_swapchain();
    if(result != VK_SUBOPTIMAL_KHR
            && !check_result("vkAcquireNextImageKHR", result))
        return false;
    update_camera(img_idx);
    update_lighting(img_idx);
    vkResetFences(this->dev.id, 1, &this->dev.swapchain.fences[this->iframe]);
    submit(img_idx);
    result = present(img_idx);
    if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        if(!this->recreate_swapchain())
            return false;
    } else if(!check_result("vkQueuePresentKHR", result))
        return false;
    prof.end();
    NNGN_PROFILE_CONTEXT(vsync);
    vkQueueWaitIdle(this->dev.present_queue);
    this->iframe = (this->iframe + 1) % this->dev.swapchain.img_views.size();
    return true;
}

}

#endif
