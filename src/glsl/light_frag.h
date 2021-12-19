#include "../const.h"

LAYOUT2(set = 0, binding = 1) uniform sampler2DArray lights_shadow_map;
#ifdef HAS_CUBE_ARRAY
LAYOUT2(set = 0, binding = 2)
    uniform samplerCubeArray lights_shadow_cube;
#else
LAYOUT2(set = 0, binding = 2)
    uniform samplerCube lights_shadow_cube[NNGN_MAX_LIGHTS];
#endif

LAYOUT(location = 1) in vec3 frag_pos;
LAYOUT(location = 2) in vec3 frag_pos_bias;
LAYOUT(location = 3) in vec3 frag_normal;
LAYOUT(location = 4) in vec3 frag_pos_light[NNGN_MAX_LIGHTS];

float attenuation(uint i, float d) {
    return 1.0f / (
        lights.point.att_cutoff[i][0]
        + lights.point.att_cutoff[i][1] * d
        + lights.point.att_cutoff[i][2] * d * d);
}

float spot(uint i, vec3 d) {
    float inner = lights.point.att_cutoff[i].w;
    if(inner == 0.0)
        return 1.0;
    const float penumbra = 0.1;
    float outer = inner - penumbra;
    float a = dot(-d, lights.point.dir[i].xyz);
    return a > outer ? clamp((a - outer) / penumbra, 0.0, 1.0) : 0.0;
}

float light_spec(float spec, vec3 dir, vec3 view_dir) {
    return spec * pow(
        max(dot(view_dir, reflect(dir, frag_normal)), 0.0f),
        32.0f);
}

float sample_shadow(uint i, vec2 v) {
    return texture(lights_shadow_map, vec3(v, i)).r;
}

float sample_shadow_point(uint i, vec3 v) {
#ifdef HAS_CUBE_ARRAY
    return texture(lights_shadow_cube, vec4(v, i)).r;
#else
    switch(i) {
    case 0u: return texture(lights_shadow_cube[0], v).r;
    case 1u: return texture(lights_shadow_cube[1], v).r;
    case 2u: return texture(lights_shadow_cube[2], v).r;
    case 3u: return texture(lights_shadow_cube[3], v).r;
    case 4u: return texture(lights_shadow_cube[4], v).r;
    case 5u: return texture(lights_shadow_cube[5], v).r;
    case 6u: return texture(lights_shadow_cube[6], v).r;
    case 7u: return texture(lights_shadow_cube[7], v).r;
    };
#endif
}

bool in_shadow(uint i) {
    if((lights.flags & NNGN_SHADOWS_ENABLED_BIT) == 0u)
        return false;
    vec3 p = frag_pos_light[i] * 0.5 + 0.5;
    return p.z > sample_shadow(i, p.xy);
}

bool in_shadow_point(uint i) {
    if((lights.flags & NNGN_SHADOWS_ENABLED_BIT) == 0u)
        return false;
    vec3 dir = frag_pos_bias - lights.point.pos[i].xyz;
    vec3 abs_dir = abs(dir);
    float z = max(abs_dir.x, max(abs_dir.y, abs_dir.z));
    z = lights.depth_transform0 - lights.depth_transform1 / z;
    return z > sample_shadow_point(i, dir);
}

vec3 light() {
    vec3 view_dir = normalize(lights.view_pos - frag_pos);
    vec3 light = lights.ambient;
    for(uint i = 0u; i < lights.n_dir; ++i) {
        float a = dot(frag_normal, -lights.dir.dir[i].xyz);
        if(a <= 0.0f || in_shadow(i))
            continue;
        light += lights.dir.color_spec[i].xyz * (
            a + light_spec(
                lights.dir.color_spec[i].w,
                lights.dir.dir[i].xyz,
                view_dir));
    }
    for(uint i = 0u; i < lights.n_point; ++i) {
        float a = dot(frag_normal, lights.point.pos[i].xyz - frag_pos);
        if(a <= 0.0f || in_shadow_point(i))
            continue;
        vec3 d = lights.point.pos[i].xyz - frag_pos;
        float att = attenuation(i, length(d));
        d = normalize(d);
        float diff = max(0.0f, dot(frag_normal, d));
        float spec = light_spec(lights.point.color_spec[i].w, -d, view_dir);
        light += lights.point.color_spec[i].xyz
            * att * spot(i, d) * (diff + spec);
    }
    return light;
}
