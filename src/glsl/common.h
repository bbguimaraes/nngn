#define ESCAPE
#ifdef VULKAN
ESCAPE#version 450
ESCAPE#extension GL_ARB_separate_shader_objects : enable
#define HAS_CUBE_ARRAY
#define LAYOUT(x) layout(x)
#define LAYOUT2(x, y) layout(x, y)
#define PUSH_CONSTANT(x) layout(push_constant) uniform push_const { x; }
#else
ESCAPE#version 300 es
#define LAYOUT(x)
#define LAYOUT2(x, y)
#define PUSH_CONSTANT(x) uniform x
precision highp int;
precision highp float;
precision highp sampler2DArray;
#endif
