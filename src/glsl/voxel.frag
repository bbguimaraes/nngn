#include "common.h"
#include "light_ubo.h"
#include "light_frag.h"

LAYOUT2(set = 2, binding = 0) uniform sampler2DArray tex;
LAYOUT(location = 0) in vec3 frag_tex_coord;
LAYOUT(location = 0) out vec4 out_color;

void main() {
    vec4 t = texture(tex, frag_tex_coord);
    out_color = t * vec4(light(), 1);
}
