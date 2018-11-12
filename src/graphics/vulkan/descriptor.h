#ifndef NNGN_GRAPHICS_VULKAN_DESCRIPTOR_H
#define NNGN_GRAPHICS_VULKAN_DESCRIPTOR_H

#include <cstdint>
#include <span>
#include <vector>

#include "utils/utils.h"

#include "vulkan.h"

namespace nngn {

class DescriptorPool {
public:
    NNGN_NO_COPY(DescriptorPool)
    DescriptorPool() = default;
    ~DescriptorPool();
    VkDescriptorPool id() const { return this->h; }
    void init(VkDevice dev);
    bool reset() const;
    bool recreate(
        std::uint32_t max, std::span<const VkDescriptorPoolSize> sizes);
    void destroy();
private:
    VkDevice dev = {};
    VkDescriptorPool h = {};
};

class DescriptorSets {
public:
    NNGN_NO_COPY(DescriptorSets)
    DescriptorSets() = default;
    ~DescriptorSets();
    VkDescriptorSetLayout layout() const { return this->m_layout; }
    std::span<const VkDescriptorSet> ids() const { return this->hs; }
    bool init(
        VkDevice dev, std::span<const VkDescriptorSetLayoutBinding> bindings);
    bool reset(VkDescriptorPool pool, std::uint32_t max);
    /**
     * Destroys resources associated with the sets.
     * Assumes \ref ids have already been destroyed (e.g. by resetting the
     * pool).
     */
    void destroy();
protected:
    VkDevice dev = {};
private:
    VkDescriptorSetLayout m_layout = {};
    std::vector<VkDescriptorSet> hs = {};
    std::vector<VkDescriptorType> types = {};
};

}

#endif
