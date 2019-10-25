#ifndef NNGN_MODEL_H
#define NNGN_MODEL_H

#include <string_view>
#include <vector>

#include "../graphics/graphics.h"

namespace nngn {

class Models {
public:
    enum Flag : uint8_t {
        DEDUP = 1u << 0, CALC_NORMALS = 1u << 1,
    };
    static bool load(
        std::string_view path, uint32_t tex,
        std::vector<Vertex> *vertices, std::vector<uint32_t> *indices,
        uint8_t flags = 0);
    static void calculate_normals(
        std::vector<Vertex>::iterator b_vec,
        std::vector<Vertex>::iterator i_vec,
        std::vector<Vertex>::iterator e_vec,
        std::vector<uint32_t>::const_iterator b_idx,
        std::vector<uint32_t>::const_iterator e_idx);
};

}

#endif
