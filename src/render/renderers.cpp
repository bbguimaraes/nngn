#include "renderers.h"

#include <span>

#include "utils/log.h"

namespace {

template<typename T, std::size_t N>
void read_table(std::span<T, N> s, nngn::lua::table_view t) {
    const auto ns = nngn::narrow<lua_Integer>(s.size());
    const auto n = t.size();
    if(n != ns)
        nngn::Log::l() << "invalid table size: " << n << " != " << ns << '\n';
    auto *p = s.data();
    for(lua_Integer i = 1; i <= ns; ++i)
        *p++ = t[i];
}

}

namespace nngn {

void SpriteRenderer::load(nngn::lua::table_view t) {
    NNGN_LOG_CONTEXT_CF(SpriteRenderer);
    read_table(std::span{this->size}, t["size"].get<nngn::lua::table>());
}

}
