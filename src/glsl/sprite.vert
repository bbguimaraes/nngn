#include "common.h"
#include "camera_ubo.h"
#include "light_ubo.h"
#include "light_vert.h"

LAYOUT(location = 0) in vec3 position;
LAYOUT(location = 1) in vec3 normal;
LAYOUT(location = 2) in vec3 tex_coord;
LAYOUT(location = 0) out vec3 frag_tex_coord;

#ifdef VULKAN
out gl_PerVertex {
    vec4 gl_Position;
};
#endif

void main() {
    set_frag_light_inputs(position, normal);
    gl_Position = camera.proj_view * vec4(position, 1);
    frag_tex_coord = tex_coord;
}
