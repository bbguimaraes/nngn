#include "common.h"
#include "camera_ubo.h"

LAYOUT(location = 0) in vec3 position;
LAYOUT(location = 1) in vec3 tex_coord;
LAYOUT(location = 0) out vec3 frag_color;
LAYOUT(location = 1) out vec3 frag_tex_coord;

void main() {
    vec3 pos = vec3(position.xy, 0);
    gl_Position = camera.proj_view * vec4(pos, 1);
    uint z = floatBitsToUint(position.z);
    frag_color.r = float(z >> 16);
    frag_color.g = float((z >> 8) & uint(0xff));
    frag_color.b = float(z & uint(0xff));
    frag_color /= 256.0f;
    frag_tex_coord = tex_coord;
}
