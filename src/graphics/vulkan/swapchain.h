#ifndef NNGN_GRAPHICS_VULKAN_SWAPCHAIN_H
#define NNGN_GRAPHICS_VULKAN_SWAPCHAIN_H

#include <cstdint>
#include <span>
#include <tuple>

#include "graphics/graphics.h"
#include "math/vec2.h"

#include "resource.h"
#include "vulkan.h"

namespace nngn {

class Device;
class DeviceMemory;
class Instance;

/** Presentation swap chain and associated objects. */
class SwapChain {
public:
    /**
     * Information used to present an image.
     * Returned by \ref acquire_img.
     */
    struct PresentContext {
        std::uint32_t img_idx;
        VkSemaphore wait, signal;
        VkFence fence;
    };
    NNGN_MOVE_ONLY(SwapChain)
    SwapChain(void) = default;
    ~SwapChain(void);
    VkSwapchainKHR id(void) const { return this->h; }
    VkSurfaceKHR surface(void) const { return this->m_surface; }
    /** Views for the swap chain images. */
    std::span<const VkImageView> img_views() const
        { return std::span{this->m_img_views}; }
    /** Frame buffers for each swap chain image. */
    std::span<const VkFramebuffer> frame_buffers() const
        { return std::span{this->m_frame_buffers}; }
    /** Frame buffers for each depth pass image. */
    std::span<const VkFramebuffer> depth_frame_buffers() const
        { return std::span{this->m_depth_frame_buffers}; }
    std::size_t cur_frame() const { return this->i_frame; }
    /** Changes the presentation mode used on the next \ref recreate. */
    void set_present_mode(VkPresentModeKHR m) { this->present_mode = m; }
    void set_surface(VkSurfaceKHR s) { this->m_surface = s; }
    void init(
        VkInstance inst, DeviceMemory *dev_mem,
        VkSurfaceFormatKHR format, VkFormat depth_format,
        VkPresentModeKHR mode);
    /** Destroys resources and recreate them. */
    bool recreate(
        const Instance &inst, const Device &dev, std::uint32_t n_images,
        VkRenderPass depth_pass, VkRenderPass render_pass,
        VkExtent2D extent, VkSurfaceTransformFlagBitsKHR transform,
        VkExtent2D shadow_map_size, VkExtent2D shadow_cube_size,
        std::span<const VkImageView> shadow_map,
        std::span<const VkImageView> shadow_cube);
    /** Acquires an image from the swap chain for rendering. */
    std::pair<PresentContext, VkResult> acquire_img();
private:
    bool create_img_views(const Instance &inst);
    bool create_depth_img(const Instance &inst, VkExtent2D extent);
    bool create_sync_objects(const Instance &inst);
    bool create_frame_buffers(
        const Instance &inst, VkExtent2D extent, VkRenderPass render_pass);
    bool create_depth_frame_buffers(
        const Instance &inst, VkRenderPass render_pass,
        VkExtent2D shadow_map_size, VkExtent2D shadow_cube_size,
        std::span<const VkImageView> shadow_map,
        std::span<const VkImageView> shadow_cube);
    void destroy();
    VkSwapchainKHR h = {};
    VkInstance instance = {};
    VkSurfaceKHR m_surface = {};
    VkDevice dev = {};
    DeviceMemory *dev_mem = {};
    VkSurfaceFormatKHR format = {};
    VkPresentModeKHR present_mode = {};
    std::vector<VkImageView> m_img_views = {};
    VkFormat depth_format = {};
    Image depth_img = {};
    VkImageView depth_img_view = {};
    struct {
        /** Signaled when an image has been acquired and rendering can begin. */
        std::vector<VkSemaphore> render;
        /** Signaled when an image has been presented. */
        std::vector<VkSemaphore> present;
    } semaphores = {};
    struct {
        /** Signaled when an image has been acquired and rendering can begin. */
        std::vector<VkFence> render;
        /**
         * Signaled when all rendering commands targeting an image finished.
         * Elements are a permutation of \ref render depending on the order
         * images are made available in the swap chain.
         */
        std::vector<VkFence> present;
    } fences = {};
    std::vector<VkFramebuffer> m_frame_buffers = {}, m_depth_frame_buffers = {};
    std::size_t i_frame = {};
};

/** Aggregate type for information about a surface. */
struct SurfaceInfo : Graphics::SurfaceInfo {
    VkSurfaceTransformFlagBitsKHR cur_transform = {};
    std::vector<VkSurfaceFormatKHR> formats = {};
    std::vector<Graphics::PresentMode> present_modes = {};
    /** Initializes all members.  Must be called before any other operation. */
    void init(VkSurfaceKHR s, VkPhysicalDevice d);
    /** Returns \c f if supported or a usable format for the surface. */
    VkSurfaceFormatKHR find_format(VkSurfaceFormatKHR f) const;
    /** Returns \c m if supported or a usable mode for the surface. */
    VkPresentModeKHR find_present_mode(Graphics::PresentMode m) const;
};

}

#endif
