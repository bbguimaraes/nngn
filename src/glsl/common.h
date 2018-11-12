#define ESCAPE
#ifdef VULKAN
ESCAPE#version 450
ESCAPE#extension GL_ARB_separate_shader_objects : enable
#define LAYOUT(x) layout(x)
#define LAYOUT2(x, y) layout(x, y)
#else
ESCAPE#version 300 es
#define LAYOUT(x)
#define LAYOUT2(x, y)
precision highp int;
precision highp float;
#endif
