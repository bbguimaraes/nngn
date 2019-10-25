#include "model.h"

#include <fstream>
#include <iomanip>
#include <unordered_map>

#if __has_include(<tiny_obj_loader.h>)
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
constexpr bool has_tiny_obj = true;
#else
constexpr bool has_tiny_obj = true;
#endif

#include "math/math.h"
#include "utils/log.h"
#include "utils/ranges.h"

#include "obj.h"

using nngn::u8, nngn::u32, nngn::vec3;

namespace {

// XXX
[[maybe_unused]] auto index_map = []() {
    using Vertex = nngn::Vertex;
    constexpr auto map_hash = [](const Vertex &v) {
//        return std::hash<std::string_view>{}(
//            {reinterpret_cast<const char*>(&v), sizeof(v)});
        constexpr auto hash = [](nngn::vec3 x) {
            constexpr auto comb = [](std::size_t seed, std::size_t h) {
                h += 0x9e3779b9 + (seed << 6) + (seed >> 2);
                return seed ^ h;
            };
            constexpr std::hash<float> h = {};
            std::size_t seed = 0;
            seed = comb(seed, h(x.x));
            seed = comb(seed, h(x.y));
            seed = comb(seed, h(x.z));
            return seed;
        };
        return ((hash(v.pos) ^ (hash(v.norm) << 1)) >> 1)
            ^ (hash(v.color) << 1);
    };
    constexpr auto map_cmp = [](
        const Vertex &lhs, const Vertex &rhs
    ) {
        return std::tie(lhs.pos, lhs.norm, lhs.color)
            == std::tie(rhs.pos, rhs.norm, rhs.color);
    };
    using T = std::unordered_map<
        Vertex, u32, decltype(map_hash), decltype(map_cmp)>;
    return T(16, map_hash, map_cmp);
};

#if __has_include(<tiny_obj_loader.h>)
void from_tiny_obj(
    tinyobj::attrib_t attrib, std::span<const tinyobj::shape_t> shapes,
    u32 tex, u8 flags,
    std::vector<nngn::Vertex> *vertices, std::vector<u32> *indices)
{
    const auto create_vertex = [attrib, tex](const auto &index) {
        constexpr auto copy = [](
            vec3 *dst, const auto &src, auto i, std::size_t n
        ) {
            const auto *const src_p = &src[static_cast<std::size_t>(i) * n];
            auto *const dst_p = &(*dst)[0];
            std::memcpy(dst_p, src_p, n * sizeof(*dst_p));
        };
        nngn::Vertex v = {};
        copy(&v.pos, attrib.vertices, index.vertex_index, 3);
        if(index.normal_index != -1)
            copy(&v.norm, attrib.normals, index.normal_index, 3);
        if(index.texcoord_index != -1)
            copy(&v.color, attrib.texcoords, index.texcoord_index, 2);
        v.color.y = 1.0f - v.color.y;
        v.color.z = static_cast<float>(tex);
        return v;
    };
    if(flags & nngn::Models::Flag::DEDUP)
        for(auto unique = index_map(); const auto &shape : shapes)
            for(const auto &index : shape.mesh.indices) {
                const auto v = create_vertex(index);
                auto it = unique.find(v);
                if(it == unique.end()) {
                    it = unique.insert(unique.end(), {v, vertices->size()});
                    vertices->push_back(v);
                }
                indices->push_back(it->second);
            }
    else
        for(const auto &shape : shapes)
            for(const auto &index : shape.mesh.indices) {
                // XXX iota
                indices->push_back(static_cast<u32>(vertices->size()));
                vertices->emplace_back(create_vertex(index));
            }
    if((flags & nngn::Models::Flag::CALC_NORMALS) && !attrib.normals.empty())
        nngn::Log::l() << "overriding model normals\n";
}
#endif

}

namespace nngn {

bool Models::load(
    const char *path, u32 tex, u8 flags,
    std::vector<Vertex> *vertices, std::vector<u32> *indices)
{
    NNGN_LOG_CONTEXT_CF(Models);
    NNGN_LOG_CONTEXT(path);
    const auto b_vec = vertices->size(), b_idx = indices->size();
    if constexpr(has_tiny_obj) {
#if __has_include(<tiny_obj_loader.h>)
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> mtl;
        std::string warn, err;
        if(!tinyobj::LoadObj(&attrib, &shapes, &mtl, &warn, &err, path)) {
            Log::l()
                << "warn: " << std::quoted(warn)
                << ", err: " << std::quoted(err) << '\n';
            return false;
        }
        from_tiny_obj(attrib, shapes, tex, flags, vertices, indices);
#endif
    } else {
        std::vector<vec3> vs = {}, ts = {}, ns = {};
        std::vector<std::array<zvec3, 3>> fs = {};
        if(auto f = std::ifstream{path, std::ios_base::in}; !f)
            return Log::perror("std::ifstream"), false;
        else if(!parse_obj(
            &f, std::span{owning_view{std::array<char, 4096>{}}},
            [&v = vs](const auto &x) { v.push_back(x); },
            [&v = ts](const auto &x) { v.push_back(x); },
            [&v = ns](const auto &x) { v.push_back(x); },
            [&v = fs](const auto &x) { v.push_back(x); }
        ))
            return false;
        const auto n = fs.size();
        vertices->reserve(vertices->size() + 3 * n);
        indices->resize(b_idx + 3 * n);
        std::iota(
            begin(*indices) + static_cast<std::ptrdiff_t>(b_idx),
            end(*indices), static_cast<u32>(b_idx));
        for(std::size_t i = 0; i != n; ++i)
            for(std::size_t fi = 0; fi != 3; ++fi) {
                const auto f = fs[i][fi];
                assert(f[0] < vs.size());
                Vertex v = {vs[f[0]]};
                if(!ts.empty()) {
                    assert(f[1] < ts.size());
                    v.color = ts[f[1]];
                }
                v.color[2] = static_cast<float>(tex);
                if(!ns.empty()) {
                    assert(f[2] < ns.size());
                    v.norm = ns[f[2]];
                }
                vertices->push_back(v);
            }
    }
    if(flags & Flag::CALC_NORMALS) {
        assert(!(indices->size() % 3));
        Models::calculate_normals(
            vertices->begin(),
            vertices->begin() + static_cast<std::ptrdiff_t>(b_vec),
            vertices->end(),
            indices->begin() + static_cast<std::ptrdiff_t>(b_idx),
            indices->end());
    }
    return true;
}

void Models::calculate_normals(
    std::vector<Vertex>::iterator b_vtx,
    std::vector<Vertex>::iterator i_vtx,
    std::vector<Vertex>::iterator e_vtx,
    std::vector<u32>::const_iterator b_idx,
    std::vector<u32>::const_iterator e_idx)
{
    assert(!(e_idx - b_idx) % 3);
    assert(b_vtx <= i_vtx);
    assert(i_vtx <= e_vtx);
    assert(b_idx <= e_idx);
    auto norm = std::vector<vec4>(static_cast<size_t>(e_vtx - b_vtx));
    while(b_idx != e_idx) {
        const auto i0 = *b_idx++, i1 = *b_idx++, i2 = *b_idx++;
        const auto p0 = (b_vtx + i0)->pos;
        const vec4 add = {
            Math::normalize(Math::cross(
                (b_vtx + i1)->pos - p0,
                (b_vtx + i2)->pos - p0)),
            1};
        norm[i0] += add;
        norm[i1] += add;
        norm[i2] += add;
    }
    std::for_each(
        i_vtx, e_vtx,
        [i = norm.begin() + (i_vtx - b_vtx)](auto &v) mutable {
            v.norm = i->xyz() / i->w; ++i;
        });
}

}
