#include "renderers.h"

#include <span>

#include "utils/log.h"

namespace {

template<typename T, std::size_t N>
void read_table(std::span<T, N> s, const nngn::lua::table &t) {
    const auto ns = s.size(), n = static_cast<std::size_t>(t.size());
    if(n != ns)
        nngn::Log::l() << "invalid table size: " << n << " != " << ns << '\n';
    auto *p = s.data();
    for(std::size_t i = 1; i <= ns; ++i)
        *p++ = t[i];
}

}

namespace nngn {

void SpriteRenderer::load(const nngn::lua::table &t) {
    NNGN_LOG_CONTEXT_CF(SpriteRenderer);
    read_table(std::span{this->size}, t["size"]);
    this->tex = t["tex"];
    if(const auto z = t.get<std::optional<float>>("z_off"))
        this->z_off = *z;
    else
        this->z_off = this->size.y / -2.0f;
    if(const auto s = t.get<std::optional<nngn::lua::table>>("scale")) {
        uvec2 scale = {}, coords0 = {}, coords1 = {};
        read_table(std::span{scale}, *s);
        if(const auto c = t.get<std::optional<nngn::lua::table>>("coords")) {
            coords0 = {(*c)[1], (*c)[2]};
            if(const auto x = c->get<std::optional<unsigned>>(3))
                coords1[0] = *x;
            else
                coords1[0] = coords0[0] + 1;
            if(const auto x = c->get<std::optional<unsigned>>(4))
                coords1[1] = *x;
            else
                coords1[1] = coords0[1] + 1;
        } else
            coords1 = {1, 1};
        SpriteRenderer::uv_coords(coords0, coords1, scale, &this->uv0);
    }
}

}
