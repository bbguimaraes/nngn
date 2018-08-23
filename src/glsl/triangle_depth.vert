#include "common.h"
#include "camera_ubo.h"

LAYOUT(location = 0) in vec3 position;

void main() {
    gl_Position = camera.proj_view * vec4(position, 1);
}
