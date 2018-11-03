#ifndef NNGN_GRAPHICS_VULKAN_CMD_POOL_H
#define NNGN_GRAPHICS_VULKAN_CMD_POOL_H

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "vulkan.h"

namespace nngn {

class CommandPool {
public:
    CommandPool() = default;
    ~CommandPool();
    CommandPool(const CommandPool &rhs) = delete;
    CommandPool &operator=(const CommandPool &rhs) = delete;
    CommandPool(CommandPool &&rhs) noexcept { *this = std::move(rhs); }
    CommandPool &operator=(CommandPool &&rhs) noexcept;
    VkCommandPool id() const { return this->h; }
    std::span<const VkCommandBuffer> buffers() const { return this->m_buffers; }
    bool init(
        VkDevice dev, std::uint32_t queue_family,
        VkCommandPoolCreateFlagBits flags = {});
    /** Allocates \c n command buffers, which are appended to \ref buffers. */
    bool alloc(std::size_t n);
    /** Resets the pool and allocates \c n command buffers. */
    bool realloc(std::size_t n);
    void reset();
    /** Deallocates and frees all commands allocated from the pool. */
    void free();
private:
    bool alloc(std::size_t n, VkCommandBuffer *p);
    void free(std::size_t n, VkCommandBuffer *p);
    VkDevice dev = {};
    VkCommandPool h = {};
    std::vector<VkCommandBuffer> m_buffers = {};
};

inline CommandPool &CommandPool::operator=(CommandPool &&rhs) noexcept {
    this->dev = std::exchange(rhs.dev, {});
    this->h = std::exchange(rhs.h, {});
    this->m_buffers = std::exchange(rhs.m_buffers, {});
    return *this;
}

}

#endif
