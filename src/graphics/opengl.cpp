#include "os/platform.h"
#include "utils/log.h"

#include "graphics.h"

#ifndef NNGN_PLATFORM_HAS_OPENGL

static std::unique_ptr<nngn::Graphics> f()
    { nngn::Log::l() << "compiled without OpenGL support\n"; return {}; }

namespace nngn {

template<> std::unique_ptr<Graphics> graphics_create_backend
    <Graphics::Backend::OPENGL_BACKEND>(const void*) { return f(); }

template<> std::unique_ptr<Graphics> graphics_create_backend
    <Graphics::Backend::OPENGL_ES_BACKEND>(const void*) { return f(); }

}

#else

#ifdef NNGN_PLATFORM_EMSCRIPTEN
#define GLFW_INCLUDE_ES3
#else
#include <GL/glew.h>
#endif
#include <GLFW/glfw3.h>

#include "timing/profile.h"
#include "utils/flags.h"

#include "glfw.h"

using nngn::u8, nngn::u32;

namespace {

class OpenGLBackend final : public nngn::GLFWBackend {
    enum Flag : u8 {
        ES = 1u << 0, CALLBACK_ERROR = 1u << 1,
    };
    nngn::Flags<Flag> flags = {};
    int maj = 0, min = 0;
public:
    OpenGLBackend(const OpenGLParameters &params, bool es);
    Version version() const final;
    bool init() final;
    bool error() final { return this->flags.is_set(Flag::CALLBACK_ERROR); }
    bool render() final;
    bool vsync() final;
};

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

bool check_result(const char *func_name) {
    const auto err = glGetError();
    if(err == GL_NO_ERROR)
        return true;
    nngn::Log::l() << func_name << ": " << gl_strerror(err) << std::endl;
    return false;
}

#define LOG_RESULT(f, ...) (f(__VA_ARGS__), check_result(#f))
#define CHECK_RESULT(f, ...) \
    do { f(__VA_ARGS__); if(!check_result(#f)) return false; } while(0)

OpenGLBackend::OpenGLBackend(const OpenGLParameters &p, bool es)
    : GLFWBackend(p), maj(p.maj), min(p.min)
    { if(es) flags.set(Flag::ES); }

auto OpenGLBackend::version() const -> Version {
    int mj = 0, mn = 0;
    if([&mj, &mn]() {
        return LOG_RESULT(glGetIntegerv, GL_MAJOR_VERSION, &mj)
            && LOG_RESULT(glGetIntegerv, GL_MINOR_VERSION, &mn);
    }())
        return {
            static_cast<u32>(mj), static_cast<u32>(mn), 0,
            this->flags & Flag::ES ? "ES" : ""};
    return {0, 0, 0, "error"};
}

bool OpenGLBackend::init() {
    NNGN_LOG_CONTEXT_CF(OpenGLBackend);
    if(!this->init_glfw())
        return false;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, this->maj);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, this->min);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    if(this->flags & Flag::ES)
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    else
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    if(this->maj >= 4 || (this->maj == 3 && this->min >= 2))
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    if(!this->create_window())
        return false;
    glfwMakeContextCurrent(this->w);
#ifdef GLEW_VERSION
    glewExperimental = GL_TRUE;
    auto error = glewInit();
    if(error != GLEW_OK) {
        nngn::Log::l()
            << "glewInit: " << glewGetErrorString(error) << std::endl;
        return false;
    }
#endif
#ifdef GL_VERSION_4_3
    if(this->params.flags.is_set(Parameters::Flag::DEBUG)) {
        glEnable(GL_DEBUG_OUTPUT);
        glDebugMessageCallback([](
            GLenum src, GLenum type, auto, GLenum severity,
            GLsizei len, const GLchar *msg, const void *p) {
                nngn::Log::l()
                    << "opengl_debug_callback("
                    << gl_enum_str(src) << ", " << gl_enum_str(type) << ", "
                    << gl_enum_str(severity) << "): "
                    << std::string_view{msg, static_cast<std::size_t>(len)}
                    << '\n';
                if(severity <= GL_DEBUG_SEVERITY_MEDIUM)
                    static_cast<OpenGLBackend*>(const_cast<void*>(p))
                        ->flags.set(Flag::CALLBACK_ERROR);
            }, this);
    }
#endif
    CHECK_RESULT(glClearColor, 0, 0, 0, 0);
    if(!this->params.flags.is_set(Parameters::Flag::HIDDEN))
        glfwShowWindow(this->w);
    return true;
}

bool OpenGLBackend::render() {
    NNGN_LOG_CONTEXT_CF(OpenGLBackend);
    auto prof = nngn::Profile::context<nngn::Profile>(
        &nngn::Profile::stats.render);
    CHECK_RESULT(glClear, GL_COLOR_BUFFER_BIT);
    prof.end();
    return true;
}

bool OpenGLBackend::vsync() {
    NNGN_PROFILE_CONTEXT(vsync);
    glfwSwapBuffers(this->w);
    return true;
}

}

namespace nngn {

template<> std::unique_ptr<Graphics> graphics_create_backend
        <Graphics::Backend::OPENGL_BACKEND>(const void *params) {
    return std::make_unique<OpenGLBackend>(
        *static_cast<const Graphics::OpenGLParameters*>(params), false);
}

template<> std::unique_ptr<Graphics> graphics_create_backend
        <Graphics::Backend::OPENGL_ES_BACKEND>(const void *params) {
    return std::make_unique<OpenGLBackend>(
        *static_cast<const Graphics::OpenGLParameters*>(params), true);
}

}

#endif
