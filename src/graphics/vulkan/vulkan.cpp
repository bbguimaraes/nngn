#include "graphics/graphics.h"
#include "os/platform.h"
#include "utils/log.h"

static constexpr auto Backend = nngn::Graphics::Backend::VULKAN_BACKEND;

#ifndef NNGN_PLATFORM_HAS_VULKAN

namespace nngn {

template<>
std::unique_ptr<Graphics> graphics_create_backend<Backend>(const void*) {
    NNGN_LOG_CONTEXT_F();
    nngn::Log::l() << "compiled without Vulkan support\n";
    return nullptr;
}

}

#else

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <limits>
#include <ranges>
#include <span>
#include <vector>

#include "vulkan.h"

#include <GLFW/glfw3.h>

#include "graphics/glfw.h"
#include "graphics/shaders.h"
#include "math/math.h"
#include "timing/profile.h"
#include "utils/flags.h"

#include "cmd_pool.h"
#include "descriptor.h"
#include "device.h"
#include "instance.h"
#include "memory.h"
#include "resource.h"
#include "staging.h"
#include "swapchain.h"
#include "utils.h"

using namespace std::string_literals;
using namespace std::string_view_literals;

using nngn::u8, nngn::u32, nngn::u64;

namespace {

/** Required device extensions. */
constexpr std::array DEVICE_EXTENSIONS = {
    // For negative viewport height.
    VK_KHR_MAINTENANCE1_EXTENSION_NAME,
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};
static_assert(std::ranges::is_sorted(DEVICE_EXTENSIONS, nngn::str_less));
/** Validation layers enabled in debug mode. */
constexpr std::array VALIDATION_LAYERS = {"VK_LAYER_KHRONOS_validation"};
static_assert(std::ranges::is_sorted(VALIDATION_LAYERS, nngn::str_less));
/** Preferred swap chain image format. */
constexpr VkSurfaceFormatKHR SURFACE_FORMAT = {
    .format = VK_FORMAT_B8G8R8A8_UNORM,
    .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
/** Preferred swap chain present mode. */
constexpr auto PRESENT_MODE = nngn::Graphics::PresentMode::FIFO;
/** Flags used to create all command pools. */
constexpr auto CMD_POOL_FLAGS = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
/** Maximum number of concurrent frames. */
constexpr std::size_t MAX_FRAMES = 32;

/** Adjusts depth values in clip space. */
const auto CLIP_PROJ = nngn::Math::transpose(nngn::mat4(
    1,  0,  0,  0,
    0,  1,  0,  0,
    0,  0, .5, .5,
    0,  0,  0,  1));

class Shaders {
public:
    NNGN_MOVE_ONLY(Shaders)
    Shaders(void) = default;
    ~Shaders(void);
    void init(nngn::Device *dev_) { this->dev = dev_; }
    bool init(
        const nngn::Instance &inst,
        nngn::Graphics::PipelineConfiguration::Type type,
        std::string_view vname, std::string_view fname,
        std::span<const std::uint8_t> vert,
        std::span<const std::uint8_t> frag);
    std::pair<VkShaderModule, VkShaderModule> idx(
        nngn::Graphics::PipelineConfiguration::Type type);
private:
    static constexpr auto N = static_cast<std::size_t>(
        nngn::Graphics::PipelineConfiguration::Type::MAX);
    nngn::Device *dev = {};
    std::array<std::pair<VkShaderModule, VkShaderModule>, N> v = {};
};

class Buffers {
public:
    using Type = nngn::Graphics::BufferConfiguration::Type;
    struct Configuration {
        std::string name = {};
        Type type = {};
    };
    NNGN_MOVE_ONLY(Buffers)
    Buffers(void) = default;
    ~Buffers(void);
    void init(VkDevice dev_, nngn::DeviceMemory *dev_mem_)
        { this->dev = dev_; this->dev_mem = dev_mem_; }
    nngn::Buffer &buffer(u32 b) { return this->buffers[b]; }
    std::tuple<VkBuffer, VkDeviceSize> vbo(std::size_t i, u32 b);
    std::tuple<VkBuffer, VkDeviceSize, VkDeviceSize> ebo(std::size_t i, u32 b);
    u32 create(
        const nngn::Instance &inst,
        const nngn::Graphics::BufferConfiguration &conf);
    bool resize(const nngn::Instance &inst, std::size_t n_frames);
    bool set_capacity(const nngn::Instance &inst, u32 b, VkDeviceSize n);
    void set_size(u32 b, u64 size);
    void copy(
        VkCommandBuffer cmd, u32 dst, VkBuffer src,
        VkDeviceSize dst_off, VkDeviceSize src_off, VkDeviceSize n);
private:
    VkDevice dev = {};
    nngn::DeviceMemory *dev_mem = {};
    std::vector<nngn::Buffer> buffers = {{}};
    std::vector<Configuration> conf = {{}};
    std::size_t n_frames = {};
};

class UBODescriptorSets : public nngn::DescriptorSets {
public:
    VkDeviceSize alignment() const { return this->m_alignment; }
    const std::bitset<MAX_FRAMES> &updated() const { return this->m_updated; }
    std::bitset<MAX_FRAMES> &updated() { return this->m_updated; }
    bool init(
        VkDevice dev, VkDeviceSize size, VkDeviceSize min_ubo_align,
        std::span<const VkDescriptorSetLayoutBinding> bindings);
private:
    VkDeviceSize m_alignment = {};
    std::bitset<MAX_FRAMES> m_updated = {};
};

class CameraDescriptorSets : public UBODescriptorSets {
public:
    bool init(VkDevice dev, VkDeviceSize min_ubo_align);
    VkDeviceSize size(std::size_t n_frames) const;
    std::uint32_t offset(std::size_t i) const;
    void write(VkBuffer ubo, VkDeviceSize offset) const;
private:
    static constexpr VkDeviceSize size();
};

struct RenderList {
    struct Stage {
        NNGN_MOVE_ONLY(Stage)
        Stage(void) = default;
        ~Stage(void) = default;
        explicit Stage(const nngn::Graphics::RenderList::Stage &s);
        VkPipeline pipeline = {};
        std::vector<std::pair<u32, u32>> buffers = {};
        u32 conf = {};
    };
    std::vector<Stage> normal = {};
};

class VulkanBackend final : public nngn::GLFWBackend {
    enum Flag : u8 {
        ERROR = 1u << 0,
        RECREATE_SWAPCHAIN = 1u << 1,
    };
    nngn::Flags<Flag> flags = {};
    Version m_version = {};
    LogLevel log_level;
    nngn::InstanceInfo instance_info = {};
    std::vector<nngn::DeviceInfo> m_device_infos = {};
    std::vector<nngn::DeviceMemoryInfo> m_memory_infos = {};
    nngn::SurfaceInfo m_surface_info = {};
    nngn::Instance instance = {};
    nngn::Device dev = {};
    nngn::DeviceMemory dev_mem = {};
    nngn::GraphicsStats m_stats = {};
    std::vector<VkFence> frame_fences = {};
    std::size_t n_swap_chain = {}, cur_frame = {}, n_frames = {};
    nngn::StagingBuffer stg_buffer = {};
    Buffers buffers = {};
    nngn::DedicatedBuffer ubo = {};
    nngn::DescriptorPool descriptor_pool = {};
    CameraDescriptorSets camera_descriptor_sets = {};
    Shaders shaders = {};
    VkRenderPass render_pass = {};
    VkPipelineCache pipeline_cache = {};
    VkPipelineLayout pipeline_layout = {};
    std::vector<VkPipeline> pipelines = {};
    nngn::SwapChain swap_chain = {};
    std::vector<nngn::CommandPool> cmd_pools = {};
    std::vector<PipelineConfiguration> pipeline_conf = {{}};
    ::RenderList render_list = {};
    static void error_callback(void *p)
        { static_cast<VulkanBackend*>(p)->flags.set(Flag::ERROR); };
    static bool begin_cmd(VkCommandBuffer cmd);
    static bool submit(
        VkQueue queue, VkCommandBuffer cmd, VkPipelineStageFlags dst_mask = {},
        VkSemaphore wait = {}, VkSemaphore signal = {}, VkFence fence = {});
    std::size_t last_frame() const
        { return (this->cur_frame + this->n_frames - 1) % this->n_frames; }
    VkCommandBuffer cur_cmd_buffer() const;
    void resize(int, int) final { this->flags.set(Flag::RECREATE_SWAPCHAIN); }
    std::tuple<VkPhysicalDevice, u32, u32> choose_device(std::size_t i) const;
    bool create_render_pass(VkFormat format);
    bool create_pipelines();
    bool recreate_swapchain();
    bool update_render_list();
    bool create_cmd_buffer(std::size_t img_idx);
public:
    NNGN_MOVE_ONLY(VulkanBackend)
    explicit VulkanBackend(const VulkanParameters &p);
    ~VulkanBackend(void) final;
    Version version(void) const final;
    bool init_backend(void) final;
    bool init_instance(void) final;
    bool init_device(void) final { return this->init_device(SIZE_MAX); }
    bool init_device(std::size_t i) final;
    std::size_t n_extensions(void) const final;
    std::size_t n_layers(void) const final;
    std::size_t n_devices(void) const final;
    std::size_t n_device_extensions(std::size_t i) const final;
    std::size_t n_queue_families(std::size_t i) const override;
    std::size_t n_present_modes(void) const final;
    std::size_t n_heaps(std::size_t i) const final;
    std::size_t n_memory_types(std::size_t ih, std::size_t i) const final;
    std::size_t selected_device(void) const final;
    void extensions(Extension *p) const final;
    void layers(Layer *p) const final;
    void device_infos(DeviceInfo *p) const final;
    void device_extensions(std::size_t i, Extension *p) const final;
    void queue_families(std::size_t i, QueueFamily *p) const override;
    SurfaceInfo surface_info() const final;
    void present_modes(PresentMode *p) const final;
    void heaps(std::size_t i, MemoryHeap *p) const final;
    void memory_types(std::size_t, std::size_t, MemoryType*) const final;
    bool error() final { return this->flags.is_set(Flag::ERROR); }
    nngn::GraphicsStats stats() override;
    bool set_n_frames(std::size_t n) final;
    bool set_n_swap_chain_images(std::size_t n) final;
    void set_swap_interval(int i) final;
    u32 create_pipeline(const PipelineConfiguration &conf) final;
    u32 create_buffer(const BufferConfiguration &conf) final;
    bool set_buffer_capacity(u32 b, u64 size) final;
    bool set_buffer_size(u32, u64 size) final;
    bool write_to_buffer(
        u32 b, u64 offset, u64 n, u64 size,
        void *data, void f(void*, void*, u64, u64)) final;
    bool set_render_list(const RenderList &l) final;
    bool render() final;
    bool vsync() final;
};

bool check_support(
    std::ranges::input_range auto &&s0,
    std::ranges::input_range auto &&s1,
    const char *name
) {
    std::vector<const char*> diff = {};
    std::ranges::set_difference(
        s0, vk_names(s1), std::back_inserter(diff), nngn::str_less);
    if(diff.empty())
        return true;
    auto &l = nngn::Log::l();
    l << "unsupported " << name << ":\n";
    for(auto x : diff)
        l << "- " << x << '\n';
    return false;
}

Shaders::~Shaders() {
    if(!this->dev)
        return;
    for(auto [vi, fi] : this->v) {
        vkDestroyShaderModule(this->dev->id(), vi, nullptr);
        vkDestroyShaderModule(this->dev->id(), fi, nullptr);
    }
}

bool Shaders::init(
    const nngn::Instance &inst,
    nngn::Graphics::PipelineConfiguration::Type type,
    std::string_view vname, std::string_view fname,
    std::span<const std::uint8_t> vert, std::span<const std::uint8_t> frag
) {
    NNGN_LOG_CONTEXT_CF(Shaders);
    const auto
        vret = this->dev->create_shader(inst, vname, vert),
        fret = this->dev->create_shader(inst, fname, frag);
    if(!vret || !fret)
        return false;
    this->v[static_cast<std::size_t>(type)] = {vret, fret};
    return true;
}

std::pair<VkShaderModule, VkShaderModule> Shaders::idx(
    nngn::Graphics::PipelineConfiguration::Type type
) {
    return this->v[static_cast<std::size_t>(type)];
}

Buffers::~Buffers() {
    NNGN_LOG_CONTEXT_CF(Buffers);
    if(!this->dev)
        return;
    for(auto &x : this->buffers)
        x.destroy(this->dev, this->dev_mem);
}

std::tuple<VkBuffer, VkDeviceSize> Buffers::vbo(std::size_t i, u32 b) {
    const auto ret = this->buffers[b];
    return {ret.id(), i * ret.capacity() / this->n_frames};
}

std::tuple<VkBuffer, VkDeviceSize, VkDeviceSize>
Buffers::ebo(std::size_t i, u32 b) {
    const auto ret = this->buffers[b];
    return {ret.id(), i * ret.capacity() / this->n_frames, ret.size()};
}

u32 Buffers::create(
    const nngn::Instance &inst,
    const nngn::Graphics::BufferConfiguration &c
) {
    const auto ret = static_cast<u32>(this->buffers.size());
    this->conf.push_back({c.name ? c.name : std::string{}, c.type});
    this->buffers.emplace_back();
    if(c.size && !this->set_capacity(inst, ret, c.size))
        return {};
    return ret;
}

bool Buffers::resize(const nngn::Instance &inst, std::size_t n) {
    NNGN_LOG_CONTEXT_CF(Buffers);
    const auto old = std::exchange(this->n_frames, n);
    for(std::size_t i = 0, vn = this->buffers.size(); i < vn; ++i) {
        auto &x = this->buffers[i];
        if(!x.id() || !x.capacity())
            continue;
        const auto size = x.capacity() / old;
        x.destroy(this->dev, this->dev_mem);
        if(!this->set_capacity(inst, static_cast<u32>(i), size))
            return false;
    }
    return true;
}

bool Buffers::set_capacity(const nngn::Instance &inst, u32 i, VkDeviceSize n) {
    NNGN_LOG_CONTEXT_CF(Buffers);
    const auto &c = this->conf[i];
    auto &b = this->buffers[i];
    constexpr auto dst = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    return b.init<VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT>(
            this->dev, this->dev_mem, n * this->n_frames,
            c.type == Type::VERTEX
                ? dst | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
                : dst | VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
        && inst.set_obj_name(this->dev, b.id(), c.name);
}

void Buffers::set_size(u32 b, u64 size) {
    if(this->buffers[b].size() != size)
        this->buffers[b].set_size(size);
}

void Buffers::copy(
    VkCommandBuffer cmd, u32 dst, VkBuffer src,
    VkDeviceSize dst_off, VkDeviceSize src_off, VkDeviceSize n
) {
    NNGN_LOG_CONTEXT_CF(Buffers);
    auto &dst_b = this->buffers[dst];
    const auto off_inc = dst_b.capacity() / this->n_frames;
    VkBufferCopy copy = {.srcOffset = src_off, .dstOffset = dst_off, .size = n};
    for(std::size_t i = 0; i < this->n_frames; ++i, copy.dstOffset += off_inc)
        vkCmdCopyBuffer(cmd, src, dst_b.id(), 1, &copy);
}

bool UBODescriptorSets::init(
    VkDevice dev_, VkDeviceSize size, VkDeviceSize min_ubo_align,
    std::span<const VkDescriptorSetLayoutBinding> bindings
) {
    if(!DescriptorSets::init(dev_, bindings))
        return false;
    this->m_alignment = nngn::Math::round_up(size, min_ubo_align);
    return true;
}

constexpr VkDeviceSize CameraDescriptorSets::size() {
    constexpr std::size_t normal = 1;
    return normal;
}

VkDeviceSize CameraDescriptorSets::size(std::size_t n_frames) const {
    return CameraDescriptorSets::size() * this->alignment() * n_frames;
}

u32 CameraDescriptorSets::offset(std::size_t i) const {
    return static_cast<u32>(
        CameraDescriptorSets::size() * this->alignment() * i);
}

bool CameraDescriptorSets::init(VkDevice dev_, VkDeviceSize min_ubo_align) {
    NNGN_LOG_CONTEXT_CF(CameraDescriptorSets);
    constexpr auto stage = VK_SHADER_STAGE_VERTEX_BIT;
    return UBODescriptorSets::init(
        dev_, sizeof(nngn::CameraUBO), min_ubo_align,
        std::to_array<VkDescriptorSetLayoutBinding>({{
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
            .descriptorCount = 1,
            .stageFlags = stage,
        }}));
}

void CameraDescriptorSets::write(VkBuffer ubo, VkDeviceSize offset) const {
    NNGN_LOG_CONTEXT_CF(CameraDescriptorSets);
    vkUpdateDescriptorSets(
        this->dev, 1,
        nngn::rptr(VkWriteDescriptorSet{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = this->ids()[0],
            .dstBinding = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
            .pBufferInfo = nngn::rptr(VkDescriptorBufferInfo{
                .buffer = ubo,
                .offset = offset,
                .range = sizeof(nngn::CameraUBO)}),
        }),
        0, nullptr);
}

RenderList::Stage::Stage(const nngn::Graphics::RenderList::Stage &s) :
    conf{s.pipeline}
{
    this->buffers.reserve(s.buffers.size());
    for(const auto &x : s.buffers)
        this->buffers.emplace_back(x);
}

VulkanBackend::VulkanBackend(const VulkanParameters &p) :
    GLFWBackend{p},
    m_version{p.version},
    log_level{p.log_level} {}

VulkanBackend::~VulkanBackend() {
    NNGN_LOG_CONTEXT_CF(VulkanBackend);
    if(!this->dev.id())
        return;
    vkDeviceWaitIdle(this->dev.id());
    for(auto x : this->pipelines)
        vkDestroyPipeline(this->dev.id(), x, nullptr);
    vkDestroyPipelineLayout(this->dev.id(), this->pipeline_layout, nullptr);
    vkDestroyPipelineCache(this->dev.id(), this->pipeline_cache, nullptr);
    vkDestroyRenderPass(this->dev.id(), this->render_pass, nullptr);
    this->ubo.destroy(this->dev.id(), &this->dev_mem);
}

auto VulkanBackend::version() const -> Version
    { return nngn::vk_parse_version(this->instance_info.version); }
std::size_t VulkanBackend::n_extensions() const
    { return this->instance_info.extensions.size(); }
std::size_t VulkanBackend::n_layers() const
    { return this->instance_info.layers.size(); }
std::size_t VulkanBackend::n_devices() const
    { return this->m_device_infos.size(); }
std::size_t VulkanBackend::n_device_extensions(std::size_t i) const
    { return this->m_device_infos[i].extensions.size(); }
std::size_t VulkanBackend::n_queue_families(std::size_t i) const
    { return this->m_device_infos[i].queue_families.size(); }
std::size_t VulkanBackend::n_present_modes() const
    { return this->m_surface_info.present_modes.size(); }
std::size_t VulkanBackend::n_heaps(std::size_t i) const
    { return this->m_memory_infos[i].heaps.size(); }
std::size_t VulkanBackend::n_memory_types(std::size_t i, std::size_t ih) const
    { return this->m_memory_infos[i].memory_types[ih].size(); }

void VulkanBackend::extensions(Extension *p) const
    { std::ranges::copy(this->instance_info.extensions, p); }
void VulkanBackend::layers(Layer *p) const
    { std::ranges::copy(this->instance_info.layers, p); }
auto VulkanBackend::surface_info() const -> SurfaceInfo
    { return static_cast<SurfaceInfo>(this->m_surface_info); }
void VulkanBackend::device_infos(DeviceInfo *p) const
    { for(const auto &x : this->m_device_infos) *p++ = x.info; }
void VulkanBackend::device_extensions(std::size_t i, Extension *p) const
    { std::ranges::copy(this->m_device_infos[i].extensions, p); }
void VulkanBackend::queue_families(std::size_t i, QueueFamily *p) const
    { std::ranges::copy(this->m_device_infos[i].queue_families, p); }
void VulkanBackend::present_modes(PresentMode *p) const
    { std::ranges::copy(this->m_surface_info.present_modes, p); }
void VulkanBackend::heaps(std::size_t i, MemoryHeap *p) const
    { std::ranges::copy(this->m_memory_infos[i].heaps, p); }

void VulkanBackend::memory_types(
    std::size_t i, std::size_t ih, MemoryType *p
) const
    { std::ranges::copy(this->m_memory_infos[i].memory_types[ih], p); }

std::size_t VulkanBackend::selected_device() const {
    const auto &v = this->m_device_infos;
    const auto i = std::ranges::find(
        v, this->dev.physical_dev(), &nngn::DeviceInfo::dev);
    assert(i != end(v));
    return static_cast<std::size_t>(i - begin(v));
}

bool VulkanBackend::init_backend() {
    NNGN_LOG_CONTEXT_CF(VulkanBackend);
    return this->init_glfw() && this->instance_info.init();
}

bool VulkanBackend::init_instance() {
    NNGN_LOG_CONTEXT_CF(VulkanBackend);
    const auto ext = [] {
        u32 n = {};
        const char **v = glfwGetRequiredInstanceExtensions(&n);
        std::vector<const char*> ret(v, v + n);
        std::ranges::sort(ret);
        return ret;
    }();
    if(auto &v = this->m_version; !v.major && !v.minor && !v.patch) {
        v.major = VK_VERSION_MAJOR(this->instance_info.version);
        v.minor = VK_VERSION_MINOR(this->instance_info.version);
        v.patch = VK_VERSION_PATCH(this->instance_info.version);
    }
    if(!this->instance_info.check_version(this->m_version)) {
        nngn::Log::l()
            << "unsupported version: "
            << vk_version_str(this->m_version) << "\n";
        return false;
    }
    if(!check_support(ext, this->instance_info.extensions, "extensions"))
        return false;
    if(this->params.flags.is_set(Parameters::Flag::DEBUG)) {
        const bool ok = check_support(
                VALIDATION_LAYERS, this->instance_info.layers, "layers")
            && this->instance.init_debug(
                this->m_version, ext, VALIDATION_LAYERS,
                &VulkanBackend::error_callback, this, this->log_level);
        if(!ok)
            return false;
    } else if(!this->instance.init(this->m_version, ext))
        return false;
    this->instance_info.init_devices(this->instance.id());
    const auto n = this->instance_info.physical_devs.size();
    this->m_memory_infos.reserve(n);
    for(auto x : this->instance_info.physical_devs)
        this->m_memory_infos.emplace_back().init(x);
    VkSurfaceKHR surface = {};
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    const bool ok = this->create_window()
        && LOG_RESULT(glfwCreateWindowSurface,
            this->instance.id(), w, nullptr, &surface);
    if(!ok)
        return false;
    this->swap_chain.set_surface(surface);
    this->m_device_infos.reserve(n);
    for(auto x : this->instance_info.physical_devs)
        this->m_device_infos.emplace_back().init(surface, x);
    return true;
}

bool VulkanBackend::begin_cmd(VkCommandBuffer cmd) {
    return LOG_RESULT(
        vkBeginCommandBuffer, cmd, nngn::rptr(VkCommandBufferBeginInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        }));
}

bool VulkanBackend::submit(
    VkQueue queue, VkCommandBuffer cmd, VkPipelineStageFlags dst_mask,
    VkSemaphore wait, VkSemaphore signal, VkFence fence
) {
    return LOG_RESULT(
        vkQueueSubmit, queue, 1,
        nngn::rptr(VkSubmitInfo{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = !!wait,
            .pWaitSemaphores = &wait,
            .pWaitDstStageMask = &dst_mask,
            .commandBufferCount = 1,
            .pCommandBuffers = &cmd,
            .signalSemaphoreCount = !!signal,
            .pSignalSemaphores = &signal}),
        fence);
}

VkCommandBuffer VulkanBackend::cur_cmd_buffer() const {
    const auto i = this->cur_frame;
    assert(i < this->cmd_pools.size());
    const auto &p = this->cmd_pools[i];
    assert(!p.buffers().empty());
    return p.buffers()[0];
}

std::tuple<VkPhysicalDevice, u32, u32> VulkanBackend::choose_device(
    std::size_t i
) const {
    auto &v = this->instance_info.physical_devs;
    std::span<const VkPhysicalDevice> devs = {v};
    const auto sel = i != SIZE_MAX;
    if(sel) {
        assert(i < devs.size());
        devs = devs.subspan(i, 1);
    }
    i = 0;
    for(const auto n = v.size(); i < n; ++i) {
        auto *const x = v[i];
        if(!check_support(
                DEVICE_EXTENSIONS, this->m_device_infos[i].extensions,
                "device extensions"))
            continue;
        bool found = {};
        u32 graphics_family = {}, present_family = {};
        std::tie(found, graphics_family, present_family) =
            this->m_device_infos[i].find_queues();
        if(!found) {
            nngn::Log::l() << "graphics/present queue(s) not found\n";
            continue;
        }
        return {x, graphics_family, present_family};
    }
    nngn::Log::l() << (sel ? "device not suitable\n" : "no devices found\n");
    return {};
}

bool VulkanBackend::init_device(std::size_t i) {
    NNGN_LOG_CONTEXT_CF(VulkanBackend);
    const auto [physical_dev, graphics_family, present_family] =
        this->choose_device(i);
    if(!physical_dev)
        return false;
    this->m_surface_info.init(this->swap_chain.surface(), physical_dev);
    const auto surface_format =
        this->m_surface_info.find_format(SURFACE_FORMAT);
    this->swap_chain.init(
        this->instance.id(), surface_format,
        this->m_surface_info.find_present_mode(PRESENT_MODE));
    const auto layers = [this]() {
        std::span<const char *const> ret = {VALIDATION_LAYERS};
        if(!this->params.flags.is_set(Parameters::Flag::DEBUG))
            ret = {};
        return ret;
    }();
    bool ok = this->dev.init(
            this->instance, physical_dev, graphics_family, present_family,
            DEVICE_EXTENSIONS, layers, {})
        && this->dev_mem.init(
            this->instance.id(), this->dev.physical_dev(), this->dev.id());
    if(!ok)
        return false;
    this->stg_buffer.init(
        this->instance, this->dev.id(), &this->dev_mem, 1u << 20);
    this->buffers.init(this->dev.id(), &this->dev_mem);
    this->shaders.init(&this->dev);
    this->descriptor_pool.init(this->dev.id());
    VkPhysicalDeviceProperties prop = {};
    vkGetPhysicalDeviceProperties(physical_dev, &prop);
    ok = this->camera_descriptor_sets.init(
            this->dev.id(), prop.limits.minUniformBufferOffsetAlignment)
        && this->shaders.init(
            this->instance, PipelineConfiguration::Type::TRIANGLE,
            "src/glsl/vk/triangle.vert.spv"sv,
            "src/glsl/vk/triangle.frag.spv"sv,
            nngn::GLSL_VK_TRIANGLE_VERT,
            nngn::GLSL_VK_TRIANGLE_FRAG)
        && this->create_render_pass(surface_format.format);
    if(!ok)
        return false;
    const auto descriptor_layouts = std::to_array<VkDescriptorSetLayout>({
        this->camera_descriptor_sets.layout(),
    });
    ok = LOG_RESULT(
            vkCreatePipelineLayout, this->dev.id(),
            nngn::rptr(nngn::vk_create_info<VkPipelineLayout>({
                .setLayoutCount = static_cast<u32>(descriptor_layouts.size()),
                .pSetLayouts = descriptor_layouts.data(),
            })),
            nullptr, &this->pipeline_layout)
        && this->instance.set_obj_name(
            this->dev.id(), this->pipeline_layout, "pipeline_layout"sv)
        && LOG_RESULT(
            vkCreatePipelineCache, this->dev.id(),
            nngn::rptr(nngn::vk_create_info<VkPipelineCache>({})),
            nullptr, &this->pipeline_cache)
        && this->set_n_frames(4)
        && this->recreate_swapchain();
    if(!ok)
        return false;
    if(!this->params.flags.is_set(Parameters::Flag::HIDDEN))
        glfwShowWindow(this->w);
    return true;
}

bool VulkanBackend::create_render_pass(VkFormat format) {
    NNGN_LOG_CONTEXT_CF(VulkanBackend);
    const VkAttachmentDescription color = {
        .format = format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };
    const VkAttachmentReference color_ref = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    const VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_ref,
    };
    constexpr VkSubpassDependency dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask =
            VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
            | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };
    return LOG_RESULT(vkCreateRenderPass,
            this->dev.id(),
            nngn::rptr(nngn::vk_create_info<VkRenderPass>({
                .attachmentCount = 1,
                .pAttachments = &color,
                .subpassCount = 1,
                .pSubpasses = &subpass,
                .dependencyCount = 1,
                .pDependencies = &dependency,
            })),
            nullptr, &this->render_pass)
        && this->instance.set_obj_name(
            this->dev.id(), this->render_pass, "render_pass"sv);
}

bool VulkanBackend::recreate_swapchain() {
    NNGN_LOG_CONTEXT_CF(VulkanBackend);
    this->flags.clear(Flag::RECREATE_SWAPCHAIN);
    vkDeviceWaitIdle(this->dev.id());
    auto &info = this->m_surface_info;
    info.init(this->swap_chain.surface(), this->dev.physical_dev());
    return this->swap_chain.recreate(
        this->instance, this->dev,
        this->n_swap_chain
            ? nngn::narrow<u32>(this->n_swap_chain)
            : std::min(
                info.min_images + 1,
                info.max_images ? info.max_images : UINT32_MAX),
        this->render_pass,
        nngn::vk_vec_to_extent(info.cur_extent), info.cur_transform);
}

bool VulkanBackend::update_render_list() {
    if(this->pipelines.size() == this->pipeline_conf.size() - 1)
        return true;
    std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages = {{{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .pName = "main",
    }, {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pName = "main",
    }}};
    using V = nngn::Vertex;
    constexpr auto bindings = std::to_array<VkVertexInputBindingDescription>({{
        .binding = 0,
        .stride = sizeof(V),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX}});
    const auto vertex_vattrs = nngn::vk_vertex_attrs<V, &V::pos, &V::color>();
    const auto vertex_vinput = nngn::vk_vertex_input(bindings, vertex_vattrs);
    constexpr VkPipelineInputAssemblyStateCreateInfo triangle_asm = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    };
    constexpr VkViewport viewport = {.minDepth = 0, .maxDepth = 1};
    constexpr VkRect2D scissor = {};
    const VkPipelineViewportStateCreateInfo viewport_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor,
    };
    constexpr VkPipelineRasterizationStateCreateInfo rasterize_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .lineWidth = 1,
    };
    constexpr VkPipelineMultisampleStateCreateInfo ms_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .minSampleShading = 1.0f,
    };
    constexpr VkPipelineColorBlendAttachmentState color_blend_att = {
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
            | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };
    const VkPipelineColorBlendStateCreateInfo color_blending_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &color_blend_att,
    };
    constexpr auto dyn_states = std::to_array<VkDynamicState>({
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    });
    const VkPipelineDynamicStateCreateInfo dyn_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = dyn_states.size(),
        .pDynamicStates = dyn_states.data(),
    };
    VkGraphicsPipelineCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pStages = shader_stages.data(),
        .pVertexInputState = &vertex_vinput,
        .pInputAssemblyState = &triangle_asm,
        .pViewportState = &viewport_info,
        .pRasterizationState = &rasterize_info,
        .pMultisampleState = &ms_info,
        .pColorBlendState = &color_blending_info,
        .pDynamicState = &dyn_state,
        .layout = this->pipeline_layout,
        .renderPass = this->render_pass,
        .subpass = 0,
        .basePipelineIndex = -1,
    };
    this->pipelines.resize(this->pipeline_conf.size() - 1);
    for(std::size_t i = 1, n = this->pipeline_conf.size(); i < n; ++i) {
        auto &pipeline = this->pipelines[i - 1];
        if(pipeline)
            continue;
        const auto &conf = this->pipeline_conf[i];
        const auto [vert, frag] = this->shaders.idx(conf.type);
        info.stageCount = 1 + !!frag;
        shader_stages[0].module = vert;
        shader_stages[1].module = frag;
        const bool ok = LOG_RESULT(vkCreateGraphicsPipelines,
                this->dev.id(), this->pipeline_cache,
                1, &info, nullptr, &pipeline)
            && this->instance.set_obj_name(this->dev.id(), pipeline, conf.name);
        if(!ok)
            return false;
    }
    for(auto *l : {
        &this->render_list.normal,
    })
        for(auto &x : *l)
            x.pipeline = pipelines[x.conf - 1];
    return true;
}

bool VulkanBackend::create_cmd_buffer(std::size_t img_idx) {
    NNGN_LOG_CONTEXT_CF(VulkanBackend);
    const auto i = this->cur_frame;
    constexpr auto bind_descriptors = [](
        auto b, auto l, const char *name, std::uint32_t first_set,
        std::span<const VkDescriptorSet> dv,
        std::span<const std::uint32_t> ov
    ) {
        NNGN_LOG_CONTEXT(name);
        vkCmdBindDescriptorSets(
            b, VK_PIPELINE_BIND_POINT_GRAPHICS, l, first_set,
            static_cast<std::uint32_t>(dv.size()), dv.data(),
            static_cast<std::uint32_t>(ov.size()), ov.data());
    };
    const auto render = [this, i](
        auto b, const char *name,
        const VkViewport &viewport, VkRect2D scissors,
        const auto &l
    ) {
        NNGN_LOG_CONTEXT(name);
        for(auto &x : l) {
            vkCmdBindPipeline(b, VK_PIPELINE_BIND_POINT_GRAPHICS, x.pipeline);
            vkCmdSetViewport(b, 0, 1, &viewport);
            vkCmdSetScissor(b, 0, 1, &scissors);
            for(auto &[vi, ei] : x.buffers) {
                const auto &[ebo, eoff, esize] = this->buffers.ebo(i, ei);
                if(!esize)
                    continue;
                const auto &[vbo, voff] = this->buffers.vbo(i, vi);
                vkCmdBindVertexBuffers(
                    b, 0, 1, nngn::rptr(vbo), nngn::rptr(voff));
                vkCmdBindIndexBuffer(b, ebo, eoff, VK_INDEX_TYPE_UINT32);
                vkCmdDrawIndexed(
                    b, static_cast<u32>(esize / sizeof(u32)), 1, 0, 0, 0);
            }
        }
    };
    const auto record_render_pass = [
        this, bind_descriptors, render, i, img_idx
    ](
        auto b, const auto extent
    ) {
        NNGN_LOG_CONTEXT("render_pass");
        vkCmdBeginRenderPass(
            b, nngn::rptr(VkRenderPassBeginInfo{
                .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                .renderPass = this->render_pass,
                .framebuffer = this->swap_chain.frame_buffers()[img_idx],
                .renderArea = {.extent = extent},
                .clearValueCount = 1,
                .pClearValues =
                     nngn::rptr(VkClearValue{.color = {{0, 0, 0, 1}}})}),
            VK_SUBPASS_CONTENTS_INLINE);
        const auto width = this->m_surface_info.cur_extent.x;
        const auto height = this->m_surface_info.cur_extent.y;
        const VkViewport viewport = {
            .y = static_cast<float>(height),
            .width = static_cast<float>(width),
            .height = -static_cast<float>(height),
            .minDepth = 0,
            .maxDepth = 1,
        };
        const VkRect2D scissors = {{}, {width, height}};
        const auto &camera_desc = this->camera_descriptor_sets;
        bind_descriptors(
            b, this->pipeline_layout, "normal", 0,
            std::array{camera_desc.ids()[0]},
            std::array{camera_desc.offset(i)});
        render(b, "normal", viewport, scissors, this->render_list.normal);
        vkCmdEndRenderPass(b);
    };
    const auto extent = nngn::vk_vec_to_extent(this->m_surface_info.cur_extent);
    const auto b = this->cmd_pools[i].buffers()[0];
    record_render_pass(b, extent);
    return LOG_RESULT(vkEndCommandBuffer, b);
}

u32 VulkanBackend::create_pipeline(const PipelineConfiguration &conf) {
    const auto ret = static_cast<u32>(this->pipeline_conf.size());
    this->pipeline_conf.emplace_back(conf);
    return ret;
}

bool VulkanBackend::set_render_list(const RenderList &l) {
    constexpr auto f = [](auto *dst, const auto &src) {
        dst->reserve(src.size());
        for(const auto &s : src)
            dst->emplace_back(s);
    };
    f(&this->render_list.normal, l.normal);
    return this->update_render_list();
}

u32 VulkanBackend::create_buffer(const BufferConfiguration &conf) {
    NNGN_LOG_CONTEXT_CF(VulkanBackend);
    BufferConfiguration conf_ = conf;
    if(!this->flags.is_set(Parameters::Flag::DEBUG))
        conf_.name = nullptr;
    return this->buffers.create(this->instance, conf_);
}

bool VulkanBackend::write_to_buffer(
    u32 i, std::uint64_t dst_off,
    std::uint64_t n, std::uint64_t size,
    void *data, void f(void*, void*, std::uint64_t, std::uint64_t)
) {
    NNGN_LOG_CONTEXT_CF(VulkanBackend);
    const auto cmd = this->cur_cmd_buffer();
    ++this->m_stats.buffers.n_writes;
    this->m_stats.buffers.total_writes_bytes += n * size;
    return this->stg_buffer.write(
        this->cur_frame, n, size,
        [this, i, dst_off, f, data, size, cmd](
            VkBuffer src, void *p, auto src_off, auto iw, auto nw
        ) mutable {
            f(data, p, iw, nw);
            const auto wsize = nw * size;
            this->buffers.copy(
                cmd, i, src, std::exchange(dst_off, dst_off + wsize), src_off,
                wsize);
            return true;
        });
}

bool VulkanBackend::set_buffer_capacity(u32 b, u64 size) {
    NNGN_LOG_CONTEXT_CF(VulkanBackend);
    return this->buffers.set_capacity(this->instance, b, size);
}

bool VulkanBackend::set_buffer_size(u32 b, u64 size) {
    this->buffers.set_size(b, size);
    return true;
}

nngn::GraphicsStats VulkanBackend::stats() {
    // TODO ring buffer
    auto ret = std::exchange(this->m_stats, {});
    ret.staging = this->stg_buffer.stats(this->last_frame());
    return ret;
}

bool VulkanBackend::set_n_frames(std::size_t n) {
    NNGN_LOG_CONTEXT_CF(VulkanBackend);
    if(!this->cmd_pools.empty()) {
        const auto cmd = this->cur_cmd_buffer();
        vkEndCommandBuffer(cmd);
        if(!this->submit(this->dev.graphics_queue(), cmd))
            return false;
    }
    vkDeviceWaitIdle(this->dev.id());
    const auto n_pools = this->cmd_pools.size();
    if(n_pools != n) {
        this->cmd_pools.resize(n);
        const auto family = this->dev.graphics_family();
        std::string name_buf = {};
        for(std::size_t i = n_pools; i < n; ++i) {
            auto &x = this->cmd_pools[i];
            const bool ok = x.init(this->dev.id(), family, CMD_POOL_FLAGS)
                && this->instance.set_obj_name(
                    this->dev.id(), x.id(), "cmd_pool"sv, i, &name_buf)
                && x.alloc(1);
            if(!ok)
                return false;
        }
    }
    for(auto &x : std::span{this->cmd_pools}.subspan(0, n_pools))
        x.reset();
    this->ubo.destroy(this->dev.id(), &this->dev_mem);
    this->n_frames = n;
    this->cur_frame = 0;
    if(!VulkanBackend::begin_cmd(this->cur_cmd_buffer()))
        return false;
    constexpr auto host_mem =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
        | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    const u32
        n_camera_desc = 1,
        n_desc = n_camera_desc;
    const VkDeviceSize
        ubo_camera_size = this->camera_descriptor_sets.size(this->n_frames),
        ubo_size = ubo_camera_size;
    bool ok = this->ubo.init<host_mem>(
            this->dev.id(), &this->dev_mem, ubo_size,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
        && this->instance.set_obj_name(
            this->dev.id(), this->ubo.id(), "ubo"sv)
        && this->instance.set_obj_name(
            this->dev.id(), this->ubo.mem(), "ubo_mem"sv)
        && this->descriptor_pool.reset()
        && this->descriptor_pool.recreate(
            n_desc, std::to_array<VkDescriptorPoolSize>({{
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
                .descriptorCount = n_camera_desc,
            }}))
        && this->instance.set_obj_name(
            this->dev.id(), this->descriptor_pool.id(), "descriptor_pool"sv)
        && this->camera_descriptor_sets.reset(this->descriptor_pool.id(), 1)
        && this->instance.set_obj_name(
            this->dev.id(), this->camera_descriptor_sets.layout(),
            "camera_descriptor_set_layout"sv)
        && this->instance.set_obj_name(
            this->dev.id(), this->camera_descriptor_sets.ids()[0],
            "camera_descriptor_set"sv);
    if(!ok)
        return false;
    this->camera_descriptor_sets.write(this->ubo.id(), 0);
    this->frame_fences.resize(this->n_frames);
    std::ranges::fill(this->frame_fences, VkFence{});
    this->stg_buffer.resize(this->n_frames);
    return this->buffers.resize(this->instance, this->n_frames)
        && this->update_render_list();
}

bool VulkanBackend::set_n_swap_chain_images(std::size_t n) {
    NNGN_LOG_CONTEXT_CF(VulkanBackend);
    this->n_swap_chain = n;
    this->flags.set(Flag::RECREATE_SWAPCHAIN);
    return true;
}

void VulkanBackend::set_swap_interval(int i) {
    this->m_swap_interval = i;
    this->swap_chain.set_present_mode(
        i ? this->m_surface_info.find_present_mode(PRESENT_MODE)
            : VK_PRESENT_MODE_IMMEDIATE_KHR);
    this->flags |= Flag::RECREATE_SWAPCHAIN;
}

bool VulkanBackend::render() {
    NNGN_LOG_CONTEXT_CF(VulkanBackend);
    NNGN_PROFILE_CONTEXT(render);
    const auto update_camera = [this] {
        constexpr auto far = 2048.0f;
        const auto extent =
            static_cast<nngn::vec2>(this->m_surface_info.cur_extent) / 2.0f;
        this->ubo.fill(
            this->dev.id(), 0, this->n_frames,
            this->camera_descriptor_sets.alignment(),
            nngn::as_byte_span(nngn::rptr(nngn::CameraUBO{
                .proj = CLIP_PROJ * nngn::Math::ortho(
                    -extent.x, extent.x, -extent.y, extent.y, 0.0f, far),
                .view = nngn::Math::look_at<float>(
                    {0, 0, far / 2}, {}, {0, 1, 0})})));
    };
    const auto cmd = this->cur_cmd_buffer();
    const auto queue = this->dev.graphics_queue();
    if(this->flags.is_set(Flag::RECREATE_SWAPCHAIN))
        if(!this->recreate_swapchain())
            return false;
    nngn::SwapChain::PresentContext ctx = {};
    for(;;) {
        const auto [ctx_, err] = this->swap_chain.acquire_img();
        if(err == VK_ERROR_OUT_OF_DATE_KHR) {
            if(!this->recreate_swapchain())
                return false;
            continue;
        }
        if(err != VK_SUBOPTIMAL_KHR)
            if(!nngn::vk_check_result("vkAcquireNextImageKHR", err))
                return false;
        ctx = ctx_;
        break;
    }
    if(auto f = std::exchange(this->frame_fences[this->cur_frame], ctx.fence))
        vkWaitForFences(this->dev.id(), 1, &f, VK_TRUE, UINT64_MAX);
    update_camera();
    if(!this->create_cmd_buffer(ctx.img_idx))
        return false;
    vkResetFences(this->dev.id(), 1, &ctx.fence);
    if(!VulkanBackend::submit(
            queue, cmd, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            ctx.wait, ctx.signal, ctx.fence))
        return false;
    const auto present_res = vkQueuePresentKHR(
        this->dev.present_queue(),
        nngn::rptr(VkPresentInfoKHR{
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &ctx.signal,
            .swapchainCount = 1,
            .pSwapchains = nngn::rptr(this->swap_chain.id()),
            .pImageIndices = &ctx.img_idx,
        }));
    if(present_res == VK_ERROR_OUT_OF_DATE_KHR
            || present_res == VK_SUBOPTIMAL_KHR)
        return this->recreate_swapchain();
    else if(!nngn::vk_check_result("vkQueuePresentKHR", present_res))
        return false;
    return true;
}

bool VulkanBackend::vsync() {
    NNGN_LOG_CONTEXT_CF(VulkanBackend);
    NNGN_PROFILE_CONTEXT(vsync);
    const auto f = this->cur_frame = (this->cur_frame + 1) % this->n_frames;
    assert(f < this->frame_fences.size());
    if(const auto fence = std::exchange(this->frame_fences[f], {}))
        vkWaitForFences(this->dev.id(), 1, &fence, VK_TRUE, UINT64_MAX);
    this->stg_buffer.destroy(f);
    auto &pool = this->cmd_pools[f];
    pool.reset();
    return VulkanBackend::begin_cmd(pool.buffers()[0]);
}

}

namespace nngn {

template<>
std::unique_ptr<Graphics> graphics_create_backend<Backend>(const void *params) {
    return std::make_unique<VulkanBackend>(
        params
            ? *static_cast<const Graphics::VulkanParameters*>(params)
            : Graphics::VulkanParameters{});
}

}

#endif
