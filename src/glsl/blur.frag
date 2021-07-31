#include "common.h"

const int N = 5;
const float weights[N] = float[](
    0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216
);

LAYOUT2(set = 2, binding = 0) uniform sampler2D tex;
PUSH_CONSTANT(vec2 dir);

LAYOUT(location = 0) in vec2 frag_tex_coord;
LAYOUT(location = 0) out vec4 out_color;

void main() {
    vec2 dir = dir / vec2(textureSize(tex, 0));
    vec4 t = texture(tex, frag_tex_coord) * weights[0];
    for(int i = 1; i < N; ++i) {
        vec2 dir_i = dir * float(i);
        t += weights[i] * (
            texture(tex, frag_tex_coord - dir_i)
            + texture(tex, frag_tex_coord + dir_i));
    }
    out_color = t;
}
