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
    this->tex = t["tex"];
    if(const auto z = t.get<std::optional<float>>("z_off"))
        this->z_off = *z;
    else
        this->z_off = this->size.y / -2.0f;
    if(const auto s = t.get<std::optional<sol::table>>("scale")) {
        uvec2 scale = {}, coords0 = {}, coords1 = {};
        read_table(std::span{scale}, *s);
        if(const auto c = t.get<std::optional<sol::table>>("coords")) {
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

void CubeRenderer::load(const sol::stack_table &t) {
    NNGN_LOG_CONTEXT_CF(CubeRenderer);
    this->size = t["size"];
    if(const auto c = t.get<std::optional<sol::table>>("color"))
        read_table(std::span{this->color}, *c);
}

void VoxelRenderer::load(const sol::stack_table &t) {
    NNGN_LOG_CONTEXT_CF(VoxelRenderer);
    this->tex = t["tex"];
    this->uv = {};
    this->size = {};
    if(const auto tt = t.get<std::optional<sol::table>>("uv")) {
        using T = decltype(VoxelRenderer::uv);
        using VT = T::value_type;
        using ET = VT::type;
        static constexpr std::size_t N = std::tuple_size<T>() * VT::n_dim;
        static_assert(sizeof(T) == N * sizeof(ET));
        read_table(std::span{&this->uv[0][0], N}, *tt);
    }
    std::ranges::begin(this->size);
    static_assert(std::ranges::contiguous_range<decltype(this->size)>);
    if(const auto tt = t.get<std::optional<sol::table>>("size"))
        read_table(std::span{this->size}, *tt);
}

}
