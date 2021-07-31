#include "common.h"

LAYOUT2(set = 2, binding = 0) uniform sampler2D tex;
PUSH_CONSTANT(float threshold);

LAYOUT(location = 0) in vec2 frag_tex_coord;
LAYOUT(location = 0) out vec4 out_color;

void main() {
    vec4 t = texture(tex, frag_tex_coord);
    float m = max(t.r, max(t.g, t.b));
    out_color = m < threshold ? vec4(0) : t;
}
