#include "../const.h"

struct DirLights {
    vec4 dir[NNGN_MAX_LIGHTS];
    vec4 color_spec[NNGN_MAX_LIGHTS];
    mat4 mat[NNGN_MAX_LIGHTS];
};

struct PointLights {
    vec4 dir[NNGN_MAX_LIGHTS];
    vec4 color_spec[NNGN_MAX_LIGHTS];
    vec4 pos[NNGN_MAX_LIGHTS];
    vec4 att_cutoff[NNGN_MAX_LIGHTS];
};

LAYOUT2(set = 0, binding = 0) uniform Lights {
    uint flags;
    float depth_transform0, depth_transform1;
    vec3 view_pos;
    uint n_dir;
    vec3 ambient;
    uint n_point;
    DirLights dir;
    PointLights point;
} lights;
