#include "common.h"

PUSH_CONSTANT(float alpha);

LAYOUT(location = 0) in vec3 frag_color;
LAYOUT(location = 0) out vec4 out_color;

void main() {
    out_color = vec4(frag_color, alpha);
}
