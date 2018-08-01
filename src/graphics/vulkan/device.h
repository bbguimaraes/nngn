#ifndef NNGN_GRAPHICS_VULKAN_DEVICE_H
#define NNGN_GRAPHICS_VULKAN_DEVICE_H

#include <span>
#include <vector>

#include "graphics/graphics.h"

#include "vulkan.h"

namespace nngn {

class Instance;

/** Owning wrapper for a VkDevice. */
class Device {
public:
    NNGN_MOVE_ONLY(Device)
    Device(void) = default;
    /** Releases the device object. */
    ~Device(void);
    VkPhysicalDevice physical_dev(void) const { return this->ph; }
    VkDevice id(void) const { return this->h; }
    /** Family of the queue returned by \ref graphics_queue. */
    std::uint32_t graphics_family(void) const;
    /** Family of the queue returned by \ref present_queue. */
    std::uint32_t present_family(void) const;
    /** Queue used for graphics operations. */
    VkQueue graphics_queue(void) const { return this->queues.graphics; }
    /** Queue used for presentation, may be the same as \ref graphics_queue. */
    VkQueue present_queue(void) const { return this->queues.present; }
    /**
     * Creates a logical device associated with \c dev.
     * Must be called before any other operation.  All arguments must be valid
     * options for the device (\ref DeviceInfo can be used to determine if they
     * are).
     */
    bool init(
        const Instance &inst, VkPhysicalDevice physical_dev,
        std::uint32_t graphics_family, std::uint32_t present_family,
        std::span<const char *const> extensions,
        std::span<const char *const> layers,
        const VkPhysicalDeviceFeatures &features);
    VkSampler create_sampler(
        VkFilter filter, VkSamplerAddressMode addr_mode, VkBorderColor border,
        VkSamplerMipmapMode mip_mode, std::uint32_t mip_levels) const;
    /** Compiles and names a shader module from source. */
    VkShaderModule create_shader(
        const Instance &inst,
        std::string_view name, std::span<const std::uint8_t> src) const;
private:
    VkPhysicalDevice ph = {};
    VkDevice h = {};
    struct { std::uint32_t graphics, present; } families = {};
    struct { VkQueue graphics, present; } queues = {};
};

/** Aggregate type for information about a physical device. */
struct DeviceInfo {
    VkPhysicalDevice dev = {};
    Graphics::DeviceInfo info = {};
    std::vector<Graphics::Extension> extensions = {};
    std::vector<Graphics::QueueFamily> queue_families = {};
    /** Initializes all members.  Must be called before any other operation. */
    void init(VkSurfaceKHR s, VkPhysicalDevice d);
    /**
     * Finds queue families that support graphics and presentation.
     * \returns
     *     <tt>{found, graphics, present}</tt>.  If a single family supports
     *     both types of operations, it is preferred and \c graphics and \c
     *     present will have the same value.
     */
    std::tuple<bool, std::uint32_t, std::uint32_t> find_queues() const;
private:
    bool can_present(VkSurfaceKHR surface, std::size_t queue_family) const;
};

inline std::uint32_t Device::graphics_family(void) const {
    return this->families.graphics;
}

inline std::uint32_t Device::present_family(void) const {
    return this->families.present;
}

}

#endif
