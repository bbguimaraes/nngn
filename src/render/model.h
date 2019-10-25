#ifndef NNGN_MODEL_H
#define NNGN_MODEL_H

#include <string_view>
#include <vector>

#include "../graphics/graphics.h"

namespace nngn {

class Models {
public:
    enum Flag : u8 {
        DEDUP = 1u << 0, CALC_NORMALS = 1u << 1,
    };
    /**
      * Reads an OBJ file as a list of vertices/indices.
      *
      * Both `vector`s are input/output arguments: existing values will be
      * preserved, new values will be appended.
      *
      * \param vertices
      *     Filled with position/texture/normal values from the OBJ file, with
      *     one exception: the third texture coordinate of every element is set
      *     to `(float)tex`.
      * \param indices
      *     Indices into `vertices`, adjusted as necessary if it already
      *     contains data.
      */
    static bool load(
        const char *path, u32 tex, u8 flags,
        std::vector<Vertex> *vertices, std::vector<u32> *indices);
    static void calculate_normals(
        std::vector<Vertex>::iterator b_vtx,
        std::vector<Vertex>::iterator i_vtx,
        std::vector<Vertex>::iterator e_vtx,
        std::vector<u32>::const_iterator b_idx,
        std::vector<u32>::const_iterator e_idx);
};

}

#endif
