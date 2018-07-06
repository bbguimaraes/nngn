#ifndef NNGN_GRAPHICS_STATS_H
#define NNGN_GRAPHICS_STATS_H

#include <cstdint>

#include "utils/def.h"

namespace nngn {

struct GraphicsStats {
    struct Staging {
        struct {
            u32 n_allocations;
            u64 total_memory;
        } req;
        u32 n_allocations, n_reused, n_free;
        u64 total_memory;
    } staging;
    struct Buffers {
        u32 n_writes;
        u64 total_writes_bytes;
    } buffers;
};

}

#endif
