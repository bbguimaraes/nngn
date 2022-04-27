#ifndef NNGN_LUA_ALLOC_H
#define NNGN_LUA_ALLOC_H

#include <array>
#include <cstddef>
#include <tuple>

#include "utils/ranges.h"
#include "utils/utils.h"

#include "lua.h"

namespace nngn::lua {

/**
 * Tracks state allocations.
 * Allocations are tracked for each of the types described in the `lua_Alloc`
 * documentation (parameter `osize`), corresponding to the elements of the
 * static array \ref types.  Each position in the array \ref v contains the
 * allocation information for the corresponding type in \ref types.  One extra
 * position at the end (i.e. at position <tt>types.size()</tt>) holds
 * information for "other" types.
 *
 * \ref lua_alloc is a static function which can serve as `lua_Alloc` allocator.
 * Its \p d parameter should be the address of an object of this type.
 * `malloc(3)`, `realloc(3)`, and `free(3)` will be used for actual memory
 * allocations.
 */
struct alloc_info {
    /** Function to be passed to `lua_newstate`. */
    static void *lua_alloc(void *d, void *p, std::size_t s0, std::size_t s1);
    /** Each entry corresponds to the allocation information in \ref v. */
    static constexpr std::array types = {
        type::string, type::table, type::function, type::user_data,
        type::thread,
    };
    /**
     * Number of types for which allocations are tracked.
     * One extra position for "other".
     */
    static constexpr auto n_types = 1 + std::tuple_size_v<decltype(types)>;
    static_assert(is_sequence(types, to_underlying<type>));
    /**
     * Allocation information for each type.
     * `n` is the number of existing allocations, `bytes` is their total size.
     */
    struct info { std::size_t n, bytes; };
    /**
     * Total size in bytes of active allocations for each type.
     * Types are in the same order as \ref types.
     */
    std::array<info, n_types> v = {};
};

}

#endif
