#ifndef NNGN_GRAPHICS_VULKAN_STAGING_H
#define NNGN_GRAPHICS_VULKAN_STAGING_H

#include <limits>

#include "graphics/stats.h"
#include "utils/log.h"

#include "instance.h"
#include "resource.h"
#include "vulkan.h"

namespace nngn {

/**
 * Manages staging buffers for a group of frames.
 * Allocates buffers and device memory in blocks as necessary for each new data
 * block that is written.
 */
class StagingBuffer {
public:
    using Stats = GraphicsStats::Staging;
    NNGN_MOVE_ONLY(StagingBuffer)
    StagingBuffer(void) = default;
    /** Releases all buffers and device memory allocated. */
    ~StagingBuffer(void);
    void init(
        const Instance &inst, VkDevice dev, DeviceMemory *dev_mem,
        VkDeviceSize default_size);
    Stats stats(std::size_t i) const;
    /** Resizes to accommodate \c n frames. */
    void resize(std::size_t n);
    /**
     * Writes data for a frame, allocating extra memory as necessary.
     * \param i frame index
     * \param n number of elements
     * \param size size of each element
     * \param f
     *     Callable that generates the data.  Parameters:
     *     - <tt>VkBuffer dst</tt>: target buffer
     *     - <tt>void *p</tt>: memory block
     *     - <tt>VkDeviceSize off</tt>: offset in \c dst
     *     - <tt>VkDeviceSize i</tt>: base index (<tt><= n</tt>)
     *     - <tt>VkDeviceSize nw</tt>: number of elements (<tt><= n - i</tt>)
     */
    bool write(std::size_t i, VkDeviceSize n, VkDeviceSize size, auto &&f);
    /** Releases resources for frame \c i. */
    void destroy(std::size_t i);
private:
    /** Buffer in the free list, ready to be reused. */
    struct FreeBuffer : DedicatedBuffer { u8 age = {}; };
    /** Data for a single frame. */
    struct Frame {
        void release(VkDevice dev, std::vector<FreeBuffer> *free);
        void destroy(VkDevice dev, DeviceMemory *dev_mem);
        std::vector<DedicatedBuffer> blocks = {};
        void *mapped = {};
        u32 n_reused = 0, n_alloc = 0, n_req_alloc = 0;
        u64 req_mem = 0;
    };
    /** Result of a block allocation. */
    struct Allocation {
        DedicatedBuffer *block = {};
        VkDeviceSize offset = {}, n = {};
    };
    /** Region mapped for writting. */
    struct MappedRegion {
        VkDeviceMemory mem = {};
        VkDeviceSize offset = {}, n = {};
    };
    const DedicatedBuffer *cur_block(std::size_t i) const;
    DedicatedBuffer *cur_block(std::size_t i);
    /**
     * Allocates at most \c n blocks of \c size bytes for frame \c i.
     * Depending on the capacity \c cap of the current active block:
     * 1. if <tt>cap >= n * size</tt>, no allocation is performed
     * 2. if <tt>size <= cap && cap < n * size</tt>, no allocation is performed
     * 3. if<tt>cap < size</tt>, a new block of size <tt>n * size</tt> is
     *   allocated
     * In all cases, the returned allocation information has \c n equal to the
     * number of blocks that should be written to the buffer, which may be less
     * than the \c n parameter for case 2.
     */
    Allocation alloc(std::size_t i, VkDeviceSize n, VkDeviceSize size);
    /**
     * Maps a writeable region, allocating if necessary.
     * \param i frame index
     * \param n number of elements
     * \param size size of each element
     */
    MappedRegion map(std::size_t i, VkDeviceSize n, VkDeviceSize size);
    /**
     * Removes old unused buffers from the free list.
     * A buffer is removed if it has not been used for a full cycle of the total
     * number of frames (i.e. `size(this->frames)`) since it was allocated.
     * Regardless of age, a minimum of `size(this->frames)` is kept in the list.
     */
    void retire_free();
    std::vector<Frame> frames = {};
    std::vector<FreeBuffer> free = {};
    const Instance *inst = {};
    VkDevice dev = {};
    DeviceMemory *dev_mem = {};
    VkDeviceSize default_size = {};
};

inline void StagingBuffer::init(
    const Instance &inst_, VkDevice dev_, DeviceMemory *dev_mem_,
    VkDeviceSize default_size_
) {
    this->inst = &inst_;
    this->dev = dev_;
    this->dev_mem = dev_mem_;
    this->default_size = default_size_;
}

inline void StagingBuffer::resize(std::size_t n) {
    for(std::size_t i = n, old = this->frames.size(); i < old; ++i)
        this->destroy(i);
    this->frames.resize(n);
}

inline const DedicatedBuffer *StagingBuffer::cur_block(
    std::size_t i
) const {
    assert(i < this->frames.size());
    auto &f = this->frames[i];
    return f.blocks.empty() ? nullptr : &f.blocks.back();
}

inline DedicatedBuffer *StagingBuffer::cur_block(std::size_t i) {
    return const_cast<DedicatedBuffer*>(
        static_cast<const StagingBuffer*>(this)->cur_block(i));
}

inline bool StagingBuffer::write(
    std::size_t i, VkDeviceSize n, VkDeviceSize size, auto &&f
) {
    NNGN_LOG_CONTEXT_CF(StagingBuffer);
    for(std::size_t bi = 0; n;) {
        auto [mem, off, nw] = this->map(i, n, size);
        if(!mem)
            return false;
        void *const p = static_cast<char*>(this->frames[i].mapped) + off;
        if(!f(this->cur_block(i)->id(), p, off, bi, nw))
            return false;
        bi += static_cast<std::size_t>(nw);
        n -= static_cast<std::size_t>(nw);
    }
    return true;
}

}

#endif
