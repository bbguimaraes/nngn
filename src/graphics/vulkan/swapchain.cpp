#include "os/platform.h"

#ifdef NNGN_PLATFORM_HAS_VULKAN
#include "swapchain.h"

#include <cassert>
#include <ranges>

#include "utils/ranges.h"

#include "device.h"
#include "instance.h"
#include "utils.h"

using namespace std::string_view_literals;

namespace nngn {

static_assert(
    static_cast<std::uint32_t>(Graphics::PresentMode::IMMEDIATE)
    == VK_PRESENT_MODE_IMMEDIATE_KHR);
static_assert(
    static_cast<std::uint32_t>(Graphics::PresentMode::FIFO)
    == VK_PRESENT_MODE_FIFO_KHR);
static_assert(
    static_cast<std::uint32_t>(Graphics::PresentMode::FIFO_RELAXED)
    == VK_PRESENT_MODE_FIFO_RELAXED_KHR);
static_assert(
    static_cast<std::uint32_t>(Graphics::PresentMode::MAILBOX)
    == VK_PRESENT_MODE_MAILBOX_KHR);

SwapChain::~SwapChain() {
    NNGN_LOG_CONTEXT_CF(SwapChain);
    if(!this->instance)
        return;
    if(this->dev) {
        this->destroy();
        for(const auto *const v : {
            &this->semaphores.render, &this->semaphores.present
        })
            for(auto x : *v)
                vkDestroySemaphore(this->dev, x, nullptr);
        for(auto x : this->fences.render)
            vkDestroyFence(this->dev, x, nullptr);
    }
    vkDestroySurfaceKHR(this->instance, this->m_surface, nullptr);
}

void SwapChain::init(
    VkInstance inst, DeviceMemory *dev_mem_,
    VkSurfaceFormatKHR format_, VkFormat depth_format_, VkPresentModeKHR mode
) {
    NNGN_LOG_CONTEXT_CF(SwapChain);
    this->instance = inst;
    this->dev_mem = dev_mem_;
    this->format = format_;
    this->depth_format = depth_format_;
    this->present_mode = mode;
}

void SwapChain::destroy() {
    NNGN_LOG_CONTEXT_CF(SwapChain);
    for(auto *const v : {
        &this->m_frame_buffers, &this->m_depth_frame_buffers
    }) {
        for(auto x : *v)
            vkDestroyFramebuffer(this->dev, x, nullptr);
        v->clear();
    }
    vkDestroyImageView(
        this->dev, std::exchange(this->depth_img_view, {}), nullptr);
    this->depth_img.destroy(this->dev, this->dev_mem);
    for(auto x : this->m_img_views)
        vkDestroyImageView(this->dev, x, nullptr);
    this->m_img_views.clear();
    vkDestroySwapchainKHR(this->dev, std::exchange(this->h, {}), nullptr);
}

bool SwapChain::recreate(
    const Instance &inst, const Device &d, std::uint32_t n_images,
    VkRenderPass depth_pass, VkRenderPass render_pass,
    VkExtent2D extent, VkSurfaceTransformFlagBitsKHR transform,
    VkExtent2D shadow_map_size, VkExtent2D shadow_cube_size,
    std::span<const VkImageView> shadow_map,
    std::span<const VkImageView> shadow_cube
) {
    NNGN_LOG_CONTEXT_CF(SwapChain);
    this->dev = d.id();
    this->destroy();
    VkSwapchainCreateInfoKHR info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = {},
        .flags = {},
        .surface = this->m_surface,
        .minImageCount = n_images,
        .imageFormat = this->format.format,
        .imageColorSpace = this->format.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = {},
        .pQueueFamilyIndices = {},
        .preTransform = transform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = this->present_mode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE};
    if(d.graphics_queue() != d.present_queue()) {
        std::array v = {d.graphics_family(), d.present_family()};
        info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        info.queueFamilyIndexCount = v.size();
        info.pQueueFamilyIndices = v.data();
    }
    return LOG_RESULT(vkCreateSwapchainKHR, d.id(), &info, nullptr, &this->h)
        && this->create_img_views(inst)
        && this->create_depth_img(inst, extent)
        && this->create_sync_objects(inst)
        && this->create_frame_buffers(inst, extent, render_pass)
        && this->create_depth_frame_buffers(
            inst, depth_pass,
            shadow_map_size, shadow_cube_size, shadow_map, shadow_cube);
}

bool SwapChain::create_img_views(const Instance &inst) {
    NNGN_LOG_CONTEXT_CF(SwapChain);
    const auto imgs = vk_enumerate<vkGetSwapchainImagesKHR>(this->dev, this->h);
    if(!inst.set_obj_name(this->dev, std::span{imgs}, "swapchain_img"sv))
        return false;
    const auto n = imgs.size();
    this->m_img_views.resize(n);
    for(std::size_t i = 0; i < n; ++i)
        CHECK_RESULT(
            vkCreateImageView, this->dev,
            rptr(VkImageViewCreateInfo{
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .pNext = {},
                .flags = {},
                .image = imgs[i],
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = this->format.format,
                .components = {
                    .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .a = VK_COMPONENT_SWIZZLE_IDENTITY},
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1}}),
            nullptr, &this->m_img_views[i]);
    return inst.set_obj_name(
        dev, std::span{this->m_img_views}, "swapchain_img_view"sv);
}

bool SwapChain::create_depth_img(const Instance &inst, VkExtent2D extent) {
    constexpr std::uint32_t mip_levels = 1, n_layers = 1, base_layer = 0;
    NNGN_LOG_CONTEXT_CF(SwapChain);
    return this->depth_img.init<VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT>(
            this->dev, this->dev_mem, 0, VK_IMAGE_TYPE_2D, this->depth_format,
            {extent.width, extent.height, 1}, mip_levels, n_layers,
            VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
                | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT)
        && inst.set_obj_name(
            this->dev, this->depth_img.id(), "depth_img"sv)
        && inst.set_obj_name(
            this->dev, this->depth_img.mem(), "depth_img_mem"sv)
        && this->depth_img.create_view(
            this->dev, VK_IMAGE_VIEW_TYPE_2D, this->depth_format,
            VK_IMAGE_ASPECT_DEPTH_BIT, mip_levels, base_layer, n_layers,
            &this->depth_img_view)
        && inst.set_obj_name(
            this->dev, this->depth_img_view, "depth_img_view"sv);
}

bool SwapChain::create_sync_objects(const Instance &inst) {
    NNGN_LOG_CONTEXT_CF(SwapChain);
    if(!this->semaphores.render.empty())
        return true;
    const auto sem = [](const auto &i, auto d, auto n, auto *v, auto name) {
        const VkSemaphoreCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = {},
            .flags = {}};
        v->resize(n);
        for(auto &x : *v)
            CHECK_RESULT(vkCreateSemaphore, d, &info, nullptr, &x);
        return i.set_obj_name(d, std::span{*v}, name);
    };
    const auto n = this->m_img_views.size();
    const bool ok = sem(
            inst, this->dev, n,
            &this->semaphores.render, "img_available_semaphore"sv)
        && sem(
            inst, this->dev, n,
            &this->semaphores.present, "render_finished_semaphore"sv);
    if(!ok)
        return false;
    this->fences.render.resize(n);
    this->fences.present.resize(n);
    const auto fence_info = VkFenceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = {},
        .flags = VK_FENCE_CREATE_SIGNALED_BIT};
    for(auto &x : this->fences.render)
        CHECK_RESULT(vkCreateFence, this->dev, &fence_info, nullptr, &x);
    return inst.set_obj_name(
        this->dev, std::span{this->fences.render}, "swapchain_fence"sv);
}

bool SwapChain::create_frame_buffers(
    const Instance &inst, VkExtent2D extent, VkRenderPass render_pass
) {
    NNGN_LOG_CONTEXT_CF(SwapChain);
    const auto n = this->m_img_views.size();
    auto attachments =
        std::to_array<VkImageView>({{}, this->depth_img_view});
    const VkFramebufferCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = render_pass,
        .attachmentCount = attachments.size(),
        .pAttachments = attachments.data(),
        .width = extent.width,
        .height = extent.height,
        .layers = 1,
    };
    this->m_frame_buffers.resize(n);
    for(std::size_t i = 0; i < n; ++i) {
        attachments[0] = this->m_img_views[i];
        CHECK_RESULT(vkCreateFramebuffer,
            dev, &info, nullptr, &this->m_frame_buffers[i]);
    }
    return inst.set_obj_name(
        dev, std::span{this->m_frame_buffers}, "swapchain_framebuffer"sv);
}

bool SwapChain::create_depth_frame_buffers(
    const Instance &inst, VkRenderPass render_pass,
    VkExtent2D shadow_map_size, VkExtent2D shadow_cube_size,
    std::span<const VkImageView> shadow_map,
    std::span<const VkImageView> shadow_cube
) {
    NNGN_LOG_CONTEXT_CF(SwapChain);
    const auto n_map = shadow_map.size(), n_cube = shadow_cube.size();
    this->m_depth_frame_buffers.resize(n_map + n_cube);
    auto info = vk_create_info<VkFramebuffer>({
        .renderPass = render_pass,
        .attachmentCount = 1,
        .width = shadow_map_size.width,
        .height = shadow_map_size.height,
        .layers = 1,
    });
    for(std::size_t i = 0; i < n_map; ++i) {
        info.pAttachments = &shadow_map[i];
        CHECK_RESULT(vkCreateFramebuffer,
            this->dev, &info, nullptr, &this->m_depth_frame_buffers[i]);
    }
    info.width = shadow_cube_size.width;
    info.height = shadow_cube_size.height;
    for(std::size_t i = 0; i < n_cube; ++i) {
        info.pAttachments = &shadow_cube[i];
        CHECK_RESULT(vkCreateFramebuffer,
            this->dev, &info, nullptr, &this->m_depth_frame_buffers[n_map + i]);
    }
    return inst.set_obj_name(
            this->dev,
            std::span{this->m_depth_frame_buffers.data(), n_map},
            "depth_framebuffers"sv)
        && inst.set_obj_name(
            this->dev,
            std::span{this->m_depth_frame_buffers.data() + n_map, n_cube},
            "depth_cube_framebuffers"sv);
}

auto SwapChain::acquire_img() -> std::pair<PresentContext, VkResult> {
    NNGN_LOG_CONTEXT_CF(SwapChain);
    std::uint32_t img_idx = {};
    const auto i = std::exchange(
        this->i_frame,
        (this->i_frame + 1) % this->m_img_views.size());
    auto sem = this->semaphores.render[i];
    auto ret = vkAcquireNextImageKHR(
        this->dev, this->h, UINT64_MAX, sem, {}, &img_idx);
    if(ret != VK_SUCCESS && ret != VK_SUBOPTIMAL_KHR)
        return {{}, ret};
    auto &f = this->fences.present[img_idx];
    if(f)
        vkWaitForFences(this->dev, 1, &f, VK_TRUE, UINT64_MAX);
    f = this->fences.render[i];
    return {{
        .img_idx = img_idx,
        .wait = sem,
        .signal = this->semaphores.present[i],
        .fence = this->fences.render[i],
    }, VK_SUCCESS};
}

void SurfaceInfo::init(VkSurfaceKHR s, VkPhysicalDevice d) {
    this->formats = vk_enumerate<vkGetPhysicalDeviceSurfaceFormatsKHR>(d, s);
    // XXX
#ifndef __clang__
    this->present_modes =
        owning_view{
            vk_enumerate<vkGetPhysicalDeviceSurfacePresentModesKHR>(d, s)}
        | std::views::transform(vk_present_mode)
        | range_to<std::vector<Graphics::PresentMode>>{};
#else
    auto v = vk_enumerate<vkGetPhysicalDeviceSurfacePresentModesKHR>(d, s);
    this->present_modes.reserve(v.size());
    for(auto x : v)
        this->present_modes.push_back(vk_present_mode(x));
#endif
    VkSurfaceCapabilitiesKHR cap = {};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(d, s, &cap);
    this->min_images = cap.minImageCount;
    this->max_images = cap.maxImageCount;
    this->min_extent = nngn::vk_extent_to_vec(cap.minImageExtent);
    this->max_extent = nngn::vk_extent_to_vec(cap.maxImageExtent);
    this->cur_extent = nngn::vk_extent_to_vec(cap.currentExtent);
    this->cur_transform = cap.currentTransform;
}

VkSurfaceFormatKHR SurfaceInfo::find_format(VkSurfaceFormatKHR f) const {
    const auto &v = this->formats;
    const auto i = std::find_if(
        begin(v), end(v),
        [f](const auto &x)
            { return x.colorSpace == f.colorSpace && x.format == f.format; });
    return i == end(v) ? v[0] : f;
}

VkPresentModeKHR SurfaceInfo::find_present_mode(Graphics::PresentMode m) const {
    const auto &v = this->present_modes;
    return std::find(begin(v), end(v), m) == end(v)
        ? VK_PRESENT_MODE_FIFO_KHR : static_cast<VkPresentModeKHR>(m);
}

}
#endif
