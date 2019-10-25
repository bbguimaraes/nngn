#include "renderers.h"

#include <span>

#include "utils/log.h"

using nngn::uvec2;

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

std::pair<uvec2, uvec2> read_coords(nngn::lua::table_view t) {
    if(const auto oc = t["coords"].get<std::optional<nngn::lua::table>>()) {
        using O = std::optional<unsigned>;
        const auto &c = *oc;
        const uvec2 c0 = {c[1], c[2]};
        const uvec2 c1 = {
            c[3].get<O>().value_or(c0[0] + 1),
            c[4].get<O>().value_or(c0[1] + 1),
        };
        return {c0, c1};
    } else
        return {{0, 0}, {1, 1}};
}

}

namespace nngn {

void SpriteRenderer::load(nngn::lua::table_view t) {
    NNGN_LOG_CONTEXT_CF(SpriteRenderer);
    read_table(std::span{this->size}, t["size"].get<nngn::lua::table>());
    this->tex = t["tex"];
    if(const auto z = t["z_off"].get<std::optional<float>>())
        this->z_off = *z;
    else
        this->z_off = this->size.y / -2.0f;
    if(const auto s = t["scale"].get<std::optional<nngn::lua::table>>()) {
        uvec2 scale = {};
        read_table(std::span{scale}, *s);
        const auto [coords0, coords1] = read_coords(t);
        SpriteRenderer::uv_coords(
            coords0, coords1, scale,
            SpriteRenderer::uv_span(&this->uv));
    }
}

void CubeRenderer::load(const nngn::lua::table &t) {
    NNGN_LOG_CONTEXT_CF(CubeRenderer);
    this->size = t["size"];
    if(const auto c = t["color"].get<std::optional<nngn::lua::table>>())
        read_table(std::span{this->color}, *c);
}

void VoxelRenderer::load(const nngn::lua::table &t) {
    NNGN_LOG_CONTEXT_CF(VoxelRenderer);
    this->tex = t["tex"];
    this->uv = {};
    this->size = {};
    if(const auto u = t["uv"].get<std::optional<nngn::lua::table>>())
        read_table(SpriteRenderer::uv_span(&this->uv), *u);
    if(const auto s = t["size"].get<std::optional<nngn::lua::table>>())
        read_table(std::span{this->size}, *s);
}

void ModelRenderer::load(nngn::lua::table_view t) {
    this->model_flags = chain_cast<Models::Flag, lua_Integer>(t["flags"]);
    this->tex = t["tex"];
    this->obj = t["obj"].get<std::string>();
    if(const auto tr = t["trans"].get<std::optional<nngn::lua::table>>()) {
        const auto &trv = *tr;
        this->trans = {trv[1], trv[2], trv[3]};
    }
    if(const auto s = t["scale"]; /*XXX*/s.get<lua::value_view>().state() && s.get<lua::value_view>().get_type() == lua::type::table)
        this->scale = {s[1], s[2], s[3]};
    else
        this->scale = vec3(s.get(1.0f));
    if(const auto r = t["rot"].get<std::optional<nngn::lua::table>>()) {
        const auto &tr = *r;
        this->rot = {tr[1], tr[2], tr[3], tr[4]};
    }
}

}
