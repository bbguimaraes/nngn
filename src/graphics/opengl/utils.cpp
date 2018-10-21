#include "os/platform.h"

#ifdef NNGN_PLATFORM_HAS_OPENGL
#include "utils.h"

namespace nngn {

const char *gl_strerror(GLenum error) {
    switch(error) {
    case GL_NO_ERROR: return "no error";
    case GL_INVALID_ENUM: return "invalid enum";
    case GL_INVALID_VALUE: return "invalid value";
    case GL_INVALID_OPERATION: return "invalid operation";
    case GL_INVALID_FRAMEBUFFER_OPERATION:
        return "invalid framebuffer operation";
    case GL_OUT_OF_MEMORY: return "out of memory";
    default: return "unkown error";
    }
}

#ifdef GL_VERSION_4_3
const char *gl_enum_str(GLenum e) {
    switch(e) {
    case GL_DEBUG_SOURCE_API: return "api";
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM: return "window system";
    case GL_DEBUG_SOURCE_SHADER_COMPILER: return "shader compiler";
    case GL_DEBUG_SOURCE_THIRD_PARTY: return "third party";
    case GL_DEBUG_SOURCE_APPLICATION: return "application";
    case GL_DEBUG_SOURCE_OTHER: return "other";
    case GL_DEBUG_TYPE_ERROR: return "error";
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "deprecated behavior";
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: return "undefined behavior";
    case GL_DEBUG_TYPE_PORTABILITY: return "portability";
    case GL_DEBUG_TYPE_PERFORMANCE: return "performance";
    case GL_DEBUG_TYPE_MARKER: return "marker";
    case GL_DEBUG_TYPE_PUSH_GROUP: return "push group";
    case GL_DEBUG_TYPE_POP_GROUP: return "pop group";
    case GL_DEBUG_TYPE_OTHER: return "other";
    case GL_DEBUG_SEVERITY_HIGH: return "high";
    case GL_DEBUG_SEVERITY_MEDIUM: return "medium";
    case GL_DEBUG_SEVERITY_LOW: return "low";
    case GL_DEBUG_SEVERITY_NOTIFICATION: return "notification";
    default: return "unknown";
    };
}
#endif

}
#endif
