#ifndef NNGN_GRAPHICS_OPENGL_H
#define NNGN_GRAPHICS_OPENGL_H

#include "os/platform.h"

#ifdef NNGN_PLATFORM_EMSCRIPTEN
#define GLFW_INCLUDE_ES3
#else
#include <GL/glew.h>
#endif
#include <GLFW/glfw3.h>

#endif
