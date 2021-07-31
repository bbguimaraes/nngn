#include "common.h"

#include "../const.h"

LAYOUT2(set = 2, binding = 0) uniform sampler2D color_tex;
LAYOUT2(set = 2, binding = 0) uniform sampler2D bloom_tex;
LAYOUT2(set = 2, binding = 0) uniform sampler2D luminance_tex;

#ifdef VULKAN
layout(push_constant) uniform push_const {
    float exposure_scale, bloom_amount, HDR_mix;
};
#else
uniform float exposure_scale;
uniform float bloom_amount;
uniform float HDR_mix;
#endif
LAYOUT(location = 0) in vec2 frag_tex_coord;
LAYOUT(location = 0) out vec4 out_color;

const vec3 luminance_rgb = vec3(0.2126, 0.7152, 0.0722);
const float d = 1.0 / 16.0;
const float uncharted_A = 0.15;
const float uncharted_B = 0.50;
const float uncharted_C = 0.10;
const float uncharted_D = 0.20;
const float uncharted_E = 0.02;
const float uncharted_F = 0.30;

vec3 hdr_exp(vec3 x, float exposure) {
    return 1.0 - exp(-x * exposure);
}

vec3 reinhard(vec3 x) {
    return x / (1.0 + x);
}

vec3 filmic(vec3 x) {
    x = max(vec3(0.0), x - 0.004);
    x = x * (6.2 * x + 0.5) / (x * (6.2 * x + 1.7) + 0.06);
    return pow(x, vec3(2.2));
}

vec3 uncharted2_map(vec3 x) {
    const float A = uncharted_A;
    const float B = uncharted_B;
    const float C = uncharted_C;
    const float D = uncharted_D;
    const float E = uncharted_E;
    const float F = uncharted_F;
    return (x * (A * x + C * B) + D * E)
        / (x * (A * x + B) + D * F)
        - E / F;
}

vec3 uncharted2(vec3 x) {
    const float exposure_bias = 3.0;
    const float W = 11.2;
    vec3 curr = uncharted2_map(exposure_bias * x);
    vec3 white_scale = 1.0 / uncharted2_map(vec3(W));
    return curr * white_scale;
}

void main() {
    float lum = dot(texture(luminance_tex, vec2(0)).rgb, luminance_rgb);
    float exposure = exposure_scale / (NNGN_LUMINANCE_MAX * lum);
    vec4 color = texture(color_tex, frag_tex_coord);
    vec4 bloom = color + bloom_amount * texture(bloom_tex, frag_tex_coord);
    vec3 hdr = vec3(0);
    switch(3) {
    case 0: hdr = hdr_exp(bloom.rgb, exposure); break;
    case 1: hdr = reinhard(exposure * bloom.rgb); break;
    case 2: hdr = filmic(exposure * bloom.rgb); break;
    case 3: hdr = uncharted2(exposure * bloom.rgb); break;
    default: hdr = exposure * bloom.rgb; break;
    }
    out_color = mix(bloom, vec4(hdr, 1), HDR_mix);
}
