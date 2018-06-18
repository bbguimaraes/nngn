#include "../const.h"

LAYOUT(location = 1) out vec3 frag_pos;
LAYOUT(location = 2) flat out vec3 frag_normal;

void set_frag_light_inputs(vec3 p, vec3 n) {
    frag_pos = p;
    frag_normal = n;
}
