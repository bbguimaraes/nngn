#include "common.h"

LAYOUT(location = 0) in vec3 position;
LAYOUT(location = 1) in vec3 color;
LAYOUT(location = 0) out vec3 frag_color;

#ifdef VULKAN
out gl_PerVertex {
    vec4 gl_Position;
};
#endif

void main() {
    gl_Position = vec4(position, 1);
    frag_color = color;
}
