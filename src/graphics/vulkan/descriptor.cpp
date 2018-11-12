#include "os/platform.h"

#ifdef NNGN_PLATFORM_HAS_VULKAN
#include "descriptor.h"

#include <cassert>

#include "utils/log.h"
#include "utils/utils.h"

#include "utils.h"

namespace nngn {

DescriptorPool::~DescriptorPool() {
    NNGN_LOG_CONTEXT_CF(DescriptorPool);
    this->destroy();
}

void DescriptorPool::init(VkDevice dev_) {
    assert(!this->h);
    this->dev = dev_;
}

bool DescriptorPool::reset() const {
    if(!this->h)
        return true;
    NNGN_LOG_CONTEXT_CF(DescriptorPool);
    return LOG_RESULT(vkResetDescriptorPool, this->dev, this->h, {});
}

bool DescriptorPool::recreate(
    std::uint32_t max, std::span<const VkDescriptorPoolSize> sizes)
{
    NNGN_LOG_CONTEXT_CF(DescriptorPool);
    this->destroy();
    return LOG_RESULT(
        vkCreateDescriptorPool, this->dev,
        rptr(vk_create_info<VkDescriptorPool>({
            .maxSets = max,
            .poolSizeCount = static_cast<std::uint32_t>(sizes.size()),
            .pPoolSizes = sizes.data(),
        })),
        nullptr, &this->h);
}

void DescriptorPool::destroy() {
    if(!this->dev)
        return;
    NNGN_LOG_CONTEXT_CF(DescriptorPool);
    vkDestroyDescriptorPool(this->dev, std::exchange(this->h, {}), nullptr);
}

DescriptorSets::~DescriptorSets() {
    NNGN_LOG_CONTEXT_CF(DescriptorSets);
    this->destroy();
}

bool DescriptorSets::init(
    VkDevice dev_, std::span<const VkDescriptorSetLayoutBinding> bindings
) {
    NNGN_LOG_CONTEXT_CF(DescriptorSets);
    this->dev = dev_;
    CHECK_RESULT(
        vkCreateDescriptorSetLayout, dev,
        rptr(vk_create_info<VkDescriptorSetLayout>({
            .bindingCount = static_cast<std::uint32_t>(bindings.size()),
            .pBindings = bindings.data(),
        })),
        nullptr, &this->m_layout);
    this->types.resize(bindings.size());
    std::ranges::transform(
        cbegin(bindings), cend(bindings), begin(this->types),
        &VkDescriptorSetLayoutBinding::descriptorType);
    return true;
}

bool DescriptorSets::reset(VkDescriptorPool pool, std::uint32_t max) {
    NNGN_LOG_CONTEXT_CF(DescriptorSets);
    this->hs.resize(max);
    std::vector<VkDescriptorSetLayout> layouts(max, this->m_layout);
    return LOG_RESULT(
        vkAllocateDescriptorSets, this->dev,
        rptr(VkDescriptorSetAllocateInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = pool,
            .descriptorSetCount = max,
            .pSetLayouts = layouts.data(),
        }),
        this->hs.data());
}

void DescriptorSets::destroy() {
    if(!this->dev)
        return;
    vkDestroyDescriptorSetLayout(
        this->dev, std::exchange(this->m_layout, {}), nullptr);
    this->hs.clear();
}

}
#endif
