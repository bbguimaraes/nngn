#ifndef NNGN_GRAPHICS_OPENGL_UTILS_H
#define NNGN_GRAPHICS_OPENGL_UTILS_H

#include <string_view>

#include "os/platform.h"
#include "utils/log.h"
#include "utils/utils.h"

#include "opengl.h"

#ifdef GL_VERSION_4_3
constexpr GLenum NNGN_GL_BUFFER = GL_BUFFER;
constexpr GLenum NNGN_GL_PROGRAM = GL_PROGRAM;
constexpr GLenum NNGN_GL_SHADER = GL_SHADER;
constexpr GLenum NNGN_GL_VERTEX_ARRAY = GL_VERTEX_ARRAY;
#else
constexpr GLenum NNGN_GL_BUFFER = 0;
constexpr GLenum NNGN_GL_PROGRAM = 0;
constexpr GLenum NNGN_GL_SHADER = 0;
constexpr GLenum NNGN_GL_VERTEX_ARRAY = 0;
#endif

#define LOG_RESULT(f, ...) (f(__VA_ARGS__), nngn::gl_check_result(#f))
#define CHECK_RESULT(f, ...) \
    do { f(__VA_ARGS__); if(!nngn::gl_check_result(#f)) return false; } while(0)

namespace nngn {

const char *gl_strerror(GLenum error);
const char *gl_enum_str(GLenum e);
bool gl_check_result(const char *func_name);
bool gl_set_obj_name(GLenum type, GLuint obj, std::string_view name);

/** RAII-based debug group manager. */
struct GLDebugGroup {
    NNGN_MOVE_ONLY(GLDebugGroup)
    GLDebugGroup([[maybe_unused]]std::string_view s) {
#ifdef GL_EXT_debug_marker
        LOG_RESULT(glPushGroupMarkerEXT,
            static_cast<GLsizei>(s.size()), s.data());
#endif
    }
    ~GLDebugGroup(void);
};

inline GLDebugGroup::~GLDebugGroup(void) {
#ifdef GL_EXT_debug_marker
    glPopGroupMarkerEXT();
    gl_check_result("glPopGroupMarkerEXT");
#endif
}

inline bool gl_check_result([[maybe_unused]] const char *func_name) {
    if constexpr(Platform::emscripten)
        /*
         * XXX WebGLRenderingContext.getError has recently (?) become extremely
         * expensive for whatever reason (~200 -> 40 FPS), even though this has
         * an imperceptible effect on regular builds.
         */
        return true;
    const auto err = glGetError();
    if(err == GL_NO_ERROR)
        return true;
    nngn::Log::l() << func_name << ": " << gl_strerror(err) << std::endl;
    return false;
}

inline bool gl_set_obj_name(
    [[maybe_unused]] GLenum type, [[maybe_unused]] GLuint obj,
    [[maybe_unused]] std::string_view name
) {
#ifdef GL_VERSION_4_3
    CHECK_RESULT(glObjectLabel,
        type, obj, static_cast<GLsizei>(name.size()), name.data());
#endif
    return true;
}

}

#endif
