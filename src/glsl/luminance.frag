#include "common.h"

LAYOUT2(set = 2, binding = 0) uniform sampler2D tex0;
LAYOUT2(set = 2, binding = 0) uniform sampler2D tex1;

LAYOUT(location = 0) out vec4 out_color;

const float d = 1.0 / 16.0;

void main() {
    vec3 v0 = texture(tex0, vec2(0)).rgb;
    vec3 v1 = texture(tex1, vec2(0.5, 0.5) / vec2(textureSize(tex1, 0))).rgb;
    out_color = vec4(mix(v0, v1, d), 1);
}
