#include <iomanip>
#include <unordered_map>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include "math/math.h"
#include "utils/log.h"

#include "model.h"

namespace {

// XXX
auto index_map = []() {
    using Vertex = nngn::Vertex;
    constexpr auto map_hash = [](const Vertex &v) {
        return std::hash<std::string_view>{}(
            {reinterpret_cast<const char*>(&v), sizeof(v)});
        constexpr auto hash = [](nngn::vec3 x) {
            constexpr auto comb = [](std::size_t seed, std::size_t h) {
                h += 0x9e3779b9 + (seed << 6) + (seed >> 2);
                return seed ^ h;
            };
            std::hash<float> h = {};
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
        Vertex, std::uint32_t, decltype(map_hash), decltype(map_cmp)>;
    return T(16, map_hash, map_cmp);
};

}

namespace nngn {

bool Models::load(
        std::string_view path, uint32_t tex,
        std::vector<Vertex> *vertices, std::vector<uint32_t> *indices,
        uint8_t flags) {
    NNGN_LOG_CONTEXT_CF(Models);
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> mtl;
    std::string warn, err;
    if(!tinyobj::LoadObj(&attrib, &shapes, &mtl, &warn, &err, path.data())) {
        Log::l()
            << "warn: " << std::quoted(warn)
            << ", err: " << std::quoted(err) << '\n';
        return false;
    }
    const auto create_vertex = [&attrib, tex](const auto &index) {
        constexpr auto copy = [](
            vec3 *dst, const auto &src, auto i, std::size_t n
        ) {
            const auto *const src_p = &src[static_cast<std::size_t>(i) * n];
            auto *const dst_p = &(*dst)[0];
            std::memcpy(dst_p, src_p, n * sizeof(*dst_p));
        };
        Vertex v;
        copy(&v.pos, attrib.vertices, index.vertex_index, 3);
        if(index.normal_index != -1)
            copy(&v.norm, attrib.normals, index.normal_index, 3);
        if(index.texcoord_index != -1)
            copy(&v.color, attrib.texcoords, index.texcoord_index, 2);
        v.color.y = 1.0f - v.color.y;
        v.color.z = static_cast<float>(tex);
        return v;
    };
    const auto b_vec = vertices->size(), b_idx = indices->size();
    if(flags & Flag::DEDUP) {
        auto unique = index_map();
        for(const auto &shape : shapes)
            for(const auto &index : shape.mesh.indices) {
                const auto v = create_vertex(index);
                auto it = unique.find(v);
                if(it == unique.end()) {
                    it = unique.insert(unique.end(), {v, vertices->size()});
                    vertices->push_back(v);
                }
                indices->push_back(it->second);
            }
    } else
        for(const auto &shape : shapes)
            for(const auto &index : shape.mesh.indices) {
                indices->push_back(static_cast<uint32_t>(vertices->size()));
                vertices->emplace_back(create_vertex(index));
            }
    if(flags & Flag::CALC_NORMALS) {
        if(!attrib.normals.empty())
            Log::l() << "overriding model normals\n";
        assert(indices->size() % 3 == 0);
        Models::calculate_normals(
            vertices->begin(),
            vertices->begin() + static_cast<ssize_t>(b_vec),
            vertices->end(),
            indices->begin() + static_cast<ssize_t>(b_idx),
            indices->end());
    }
    return true;
}

void Models::calculate_normals(
        std::vector<Vertex>::iterator b_vec,
        std::vector<Vertex>::iterator i_vec,
        std::vector<Vertex>::iterator e_vec,
        std::vector<uint32_t>::const_iterator b_idx,
        std::vector<uint32_t>::const_iterator e_idx) {
    assert(b_vec <= i_vec);
    assert(i_vec <= e_vec);
    assert(b_idx <= e_idx);
    auto norm = std::vector<vec4>(static_cast<size_t>(e_vec - b_vec));
    while(b_idx < e_idx) {
        const auto i0 = *b_idx++, i1 = *b_idx++, i2 = *b_idx++;
        const auto p0 = (b_vec + i0)->pos;
        const vec4 add = {
            Math::normalize(Math::cross(
                (b_vec + i1)->pos - p0,
                (b_vec + i2)->pos - p0)),
            1};
        norm[i0] += add;
        norm[i1] += add;
        norm[i2] += add;
    }
    std::for_each(
        i_vec, e_vec,
        [i = norm.begin() + (i_vec - b_vec)](auto &v) mutable
            { v.norm = i->xyz() / i->w; ++i; });
}

}
