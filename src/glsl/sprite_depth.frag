#include "common.h"

LAYOUT2(set = 2, binding = 0) uniform sampler2DArray tex;
LAYOUT(location = 0) in vec3 frag_tex_coord;

void main() {
    if(texture(tex, frag_tex_coord).a == 0.0f)
        discard;
}
