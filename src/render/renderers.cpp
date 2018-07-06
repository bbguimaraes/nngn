#include "renderers.h"

#include <span>

#include <sol/table.hpp>

#include "utils/log.h"

namespace {

template<typename T, std::size_t N>
void read_table(std::span<T, N> s, const sol::table &t) {
    const auto n = t.size(), ns = s.size();
    if(n != ns)
        nngn::Log::l() << "invalid table size: " << n << " != " << ns << '\n';
    auto *p = s.data();
    for(std::size_t i = 1; i <= ns; ++i)
        *p++ = t[i];
}

}

namespace nngn {

void SpriteRenderer::load(const sol::stack_table &t) {
    NNGN_LOG_CONTEXT_CF(SpriteRenderer);
    read_table(std::span{this->size}, t["size"]);
}

}
