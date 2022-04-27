#include "alloc.h"

#include "utils/alloc/block.h"
#include "utils/alloc/realloc.h"
#include "utils/alloc/tagging.h"
#include "utils/alloc/tracking.h"
#include "utils/literals.h"
#include "utils/ranges.h"

using namespace nngn::literals;

namespace {

class allocation {
    static constexpr auto type_shift =
        std::countl_zero(nngn::lua::alloc_info::n_types);
    static constexpr auto size_mask = (1_z << type_shift) - 1_z;
public:
    using type = nngn::lua::type;
    static constexpr std::size_t to_index(type t);
    constexpr std::size_t index(void) const { return this->i >> type_shift; }
    constexpr std::size_t size(void) const { return this->i & size_mask; }
    constexpr void set(type t, std::size_t size);
    constexpr void set_size(std::size_t size);
private:
    std::size_t i;
};
static_assert(nngn::trivial<allocation>);

constexpr std::size_t allocation::to_index(type t) {
    constexpr auto max = nngn::lua::alloc_info::n_types - 1;
    constexpr auto f = nngn::to_underlying<type>;
    constexpr auto first = f(nngn::lua::alloc_info::types.front());
    return std::min(static_cast<std::size_t>(f(t) - first), max);
}

constexpr void allocation::set(type t, std::size_t size) {
    assert(!(size >> type_shift));
    this->i = (allocation::to_index(t) << type_shift) | size;
}

constexpr void allocation::set_size(std::size_t size) {
    assert(!(size >> type_shift));
    this->i = (this->i & ~size_mask) | size;
}

template<typename T>
struct tracker {
    using value_type = T;
    using pointer = std::add_pointer_t<T>;
    using block_type = nngn::alloc_block<allocation, value_type>;
    template<typename U> struct rebind { using other = tracker<U>; };
    void allocate(pointer, std::size_t) { assert(!"not implemented"); }
    void allocate(pointer p, std::size_t n, nngn::lua::type t);
    void reallocate_pre(pointer p, std::size_t n);
    void reallocate(pointer p, std::size_t n);
    void deallocate(pointer p, std::size_t n);
    nngn::lua::alloc_info *info = nullptr;
};

template<typename T>
void tracker<T>::allocate(pointer p, std::size_t n, nngn::lua::type t) {
    n *= sizeof(value_type);
    block_type::from_ptr(p).header()->set(t, n);
    const auto i = allocation::to_index(t);
    ++this->info->v[i].n;
    this->info->v[i].bytes += n;
}

template<typename T>
void tracker<T>::reallocate_pre(pointer p, std::size_t) {
    const auto *h = block_type::from_ptr(p).header();
    const auto size = h->size(), i = h->index();
    --this->info->v[i].n;
    this->info->v[i].bytes -= size;
}

template<typename T>
void tracker<T>::reallocate(pointer p, std::size_t n) {
    n *= sizeof(value_type);
    auto h = block_type::from_ptr(p).header();
    h->set_size(n);
    const auto i = h->index();
    ++this->info->v[i].n;
    this->info->v[i].bytes += n;
}

template<typename T>
void tracker<T>::deallocate(pointer p, std::size_t n) {
    n *= sizeof(value_type);
    auto h = block_type::from_ptr(p).header();
    assert(h->size() == n);
    const auto i = h->index();
    --this->info->v[i].n;
    this->info->v[i].bytes -= n;
}

}

namespace nngn::lua {

void *alloc_info::lua_alloc(void *d, void *p, std::size_t s0, std::size_t s1) {
    using T = char;
    using tracker = ::tracker<T>;
    using A0 = reallocator<T>;
    using A1 = tagging_allocator<tracker, A0>;
    using A2 = tracking_allocator<tracker, A1>;
    auto alloc = A2{tracker{static_cast<alloc_info*>(d)}};
    if(s1)
        return p
            ? alloc.reallocate(static_cast<char*>(p), s1)
            : alloc.allocate(s1, static_cast<type>(s0));
    if(p)
        alloc.deallocate(static_cast<char*>(p), s0);
    return nullptr;
}

}
