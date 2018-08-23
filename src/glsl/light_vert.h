#include "../const.h"

LAYOUT(location = 1) out vec3 frag_pos;
LAYOUT(location = 2) out vec3 frag_pos_bias;
LAYOUT(location = 3) flat out vec3 frag_normal;
LAYOUT(location = 4) out vec3 frag_pos_light[NNGN_MAX_LIGHTS];

void set_frag_light_inputs(vec3 p, vec3 n) {
    frag_pos = p;
    frag_normal = n;
    if((lights.flags & NNGN_SHADOWS_ENABLED_BIT) == 0u)
        return;
    frag_pos_bias = p + n;
    for(uint i = 0u; i < lights.n_dir; ++i) {
        vec4 v = lights.dir.mat[i] * vec4(frag_pos_bias, 1);
        frag_pos_light[i] = v.xyz / v.w;
    }
}
