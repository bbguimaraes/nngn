#include "common.h"
#include "light_ubo.h"
#include "light_frag.h"

#ifdef VULKAN
layout(push_constant) uniform push_const { float alpha; };
#else
uniform float alpha;
#endif
LAYOUT(location = 0) in vec3 frag_color;
LAYOUT(location = 0) out vec4 out_color;

void main() {
    out_color = vec4(frag_color * light(), alpha);
}
