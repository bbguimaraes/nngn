#include "os/platform.h"

#ifdef NNGN_PLATFORM_HAS_VULKAN
#include "staging.h"

#include <cassert>

#include "utils/literals.h"

using namespace std::string_view_literals;

namespace nngn {

void StagingBuffer::Frame::release(
    VkDevice dev_, std::vector<FreeBuffer> *free_
) {
    if(this->mapped)
        vkUnmapMemory(dev_, this->blocks.back().mem());
    for(auto &x : this->blocks) {
        x.set_size(0);
        free_->push_back({x});
    }
    *this = {};
}

void StagingBuffer::Frame::destroy(VkDevice dev_, DeviceMemory *dev_mem_) {
    if(this->mapped)
        vkUnmapMemory(dev_, this->blocks.back().mem());
    for(auto &x : this->blocks)
        x.destroy(dev_, dev_mem_);
    *this = {};
}

StagingBuffer::~StagingBuffer() {
    NNGN_LOG_CONTEXT_CF(StagingBuffer);
    for(auto &x : this->frames)
        x.destroy(this->dev, this->dev_mem);
    for(auto &x : this->free)
        x.destroy(this->dev, this->dev_mem);
}

void StagingBuffer::destroy(std::size_t i) {
    assert(i < this->frames.size());
    this->frames[i].release(this->dev, &this->free);
    this->retire_free();
}

void StagingBuffer::retire_free() {
    const auto n = this->frames.size();
    assert(n <= std::numeric_limits<decltype(FreeBuffer::age)>::max());
    const auto b = begin(this->free) + static_cast<std::ptrdiff_t>(n);
    const auto e = end(this->free);
    if(e < b)
        return;
    const auto i = std::find_if_not(
        b, e, [n](const auto &x) { return x.age == n; });
    std::for_each(
        b, i, [this](auto &x) { x.destroy(this->dev, this->dev_mem); });
    std::for_each(i, e, [](auto &x) { ++x.age; });
    this->free.erase(b, i);
}

auto StagingBuffer::stats(std::size_t i) const -> Stats {
    using namespace nngn::literals;
    const auto &f = this->frames[i];
    return {
        .req = {
            .n_allocations = f.n_req_alloc,
            .total_memory = f.req_mem},
        .n_allocations = f.n_alloc,
        .n_reused = f.n_reused,
        .n_free = static_cast<u32>(this->free.size()),
        .total_memory = std::accumulate(
            begin(f.blocks), end(f.blocks), 0_z,
            [](auto lhs, const auto &rhs) { return lhs + rhs.capacity(); })};
}

auto StagingBuffer::alloc(
    std::size_t i, VkDeviceSize n, VkDeviceSize size
) -> Allocation {
    NNGN_LOG_CONTEXT_CF(StagingBuffer);
    if(this->default_size < size) {
        Log::l()
            << "allocation size larger than block size ("
            << size << " >= " << this->default_size <<")\n";
        return {};
    }
    assert(i < this->frames.size());
    auto &f = this->frames[i];
    ++f.n_req_alloc;
    if(auto *const b = this->cur_block(i)) {
        const auto offset = b->size();
        if(auto cap = (b->capacity() - offset) / size) {
            cap = std::min(n, cap);
            size *= cap;
            b->set_size(offset + size);
            f.req_mem += size;
            if(void *&p = this->frames[i].mapped; !p)
                vkMapMemory(this->dev, b->mem(), 0, b->capacity(), 0, &p);
            return {.block = b, .offset = offset, .n = cap};
        } else
            vkUnmapMemory(this->dev, b->mem());
    }
    if(!this->free.empty()) {
        f.blocks.emplace_back(this->free.back());
        this->free.pop_back();
        ++f.n_reused;
    } else {
        constexpr auto host_flags =
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
            | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        ++f.n_alloc;
        auto &b = f.blocks.emplace_back();
        const bool ok = b.init<host_flags>(
                this->dev, this->dev_mem, this->default_size,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
            && this->inst->set_obj_name(this->dev, b.id(), "staging"sv)
            && this->inst->set_obj_name(this->dev, b.mem(), "staging_mem"sv);
        if(!ok)
            return {};
    }
    auto &b = f.blocks.back();
    const auto cap = std::min(n, (b.capacity() - b.size()) / size);
    size *= cap;
    b.set_size(size);
    f.req_mem += size;
    vkMapMemory(
        this->dev, b.mem(), 0, b.capacity(), 0,
        &this->frames[i].mapped);
    return {.block = &b, .offset = 0, .n = cap};
}

auto StagingBuffer::map(
    std::size_t i, VkDeviceSize n, VkDeviceSize size
) -> MappedRegion {
    auto [b, off, nw] = this->alloc(i, n, size);
    if(!b)
        return {};
    return {.mem = b->mem(), .offset = off, .n = nw};
}

}
#endif
