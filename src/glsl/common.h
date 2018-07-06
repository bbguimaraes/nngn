#define ESCAPE
#ifdef VULKAN
ESCAPE#version 450
ESCAPE#extension GL_ARB_separate_shader_objects : enable
#define LAYOUT(x) layout(x)
#else
ESCAPE#version 300 es
#define LAYOUT(x)
precision highp int;
precision highp float;
#endif
