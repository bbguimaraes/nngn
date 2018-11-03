#include "os/platform.h"

#ifdef NNGN_PLATFORM_HAS_VULKAN
#include "cmd_pool.h"

#include "utils/log.h"
#include "utils/utils.h"

#include "utils.h"

namespace nngn {

CommandPool::~CommandPool() {
    if(!this->dev)
        return;
    NNGN_LOG_CONTEXT_CF(CommandPool);
    vkDestroyCommandPool(this->dev, this->h, nullptr);
}

bool CommandPool::init(
    VkDevice dev_, std::uint32_t queue_family,
    VkCommandPoolCreateFlagBits flags
) {
    NNGN_LOG_CONTEXT_CF(CommandPool);
    this->dev = dev_;
    return LOG_RESULT(
        vkCreateCommandPool, dev_,
        rptr(VkCommandPoolCreateInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = {},
            .flags = flags,
            .queueFamilyIndex = queue_family}),
        nullptr, &this->h);
}

bool CommandPool::alloc(std::size_t n) {
    const auto old = this->m_buffers.size();
    this->m_buffers.resize(old + n);
    return this->alloc(n, this->m_buffers.data() + old);
}

bool CommandPool::alloc(std::size_t n, VkCommandBuffer *p) {
    NNGN_LOG_CONTEXT_CF(CommandPool);
    return LOG_RESULT(
        vkAllocateCommandBuffers, this->dev,
        rptr(VkCommandBufferAllocateInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = {},
            .commandPool = this->h,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = static_cast<std::uint32_t>(n)}),
        p);
}

void CommandPool::free(std::size_t n, VkCommandBuffer *p) {
    vkFreeCommandBuffers(
        this->dev, this->h, static_cast<std::uint32_t>(n), p);
}

bool CommandPool::realloc(std::size_t n) {
    NNGN_LOG_CONTEXT_CF(CommandPool);
    this->reset();
    const auto old = this->m_buffers.size();
    if(n == old)
        return true;
    this->m_buffers.resize(n);
    return n <= old || this->alloc(n - old, this->m_buffers.data() + old);
}

void CommandPool::reset() { vkResetCommandPool(this->dev, this->h, {}); }

void CommandPool::free() {
    NNGN_LOG_CONTEXT_CF(CommandPool);
    vkResetCommandPool(this->dev, this->h, {});
    if(const auto n = this->m_buffers.size()) {
        this->free(n, this->m_buffers.data());
        this->m_buffers.clear();
    }
}

}
#endif
