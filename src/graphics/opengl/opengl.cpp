#include "graphics/graphics.h"
#include "os/platform.h"
#include "utils/log.h"

static constexpr auto backend = nngn::Graphics::Backend::OPENGL_BACKEND;
static constexpr auto backend_es = nngn::Graphics::Backend::OPENGL_ES_BACKEND;

#ifndef NNGN_PLATFORM_HAS_OPENGL

static std::unique_ptr<nngn::Graphics> f() {
    nngn::Log::l() << "compiled without OpenGL support\n";
    return {};
}

namespace nngn {

template<>
std::unique_ptr<Graphics> graphics_create_backend<backend>(const void*) {
    NNGN_LOG_CONTEXT_F();
    return f();
}

template<>
std::unique_ptr<Graphics> graphics_create_backend<backend_es>(const void*) {
    NNGN_LOG_CONTEXT_F();
    return f();
}

}

#else

#include "graphics/glfw.h"
#include "timing/profile.h"
#include "utils/flags.h"

#include "opengl.h"
#include "utils.h"

using namespace std::string_view_literals;
using nngn::u8, nngn::u32;

namespace {

class OpenGLBackend final : public nngn::GLFWBackend {
    enum Flag : u8 {
        ES = 1u << 0,
        CALLBACK_ERROR = 1u << 1,
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
                    << nngn::gl_enum_str(src) << ", "
                    << nngn::gl_enum_str(type) << ", "
                    << nngn::gl_enum_str(severity) << "): "
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
    constexpr auto render_pass = [] {
        NNGN_ANON_DECL(nngn::GLDebugGroup{"color"sv});
        CHECK_RESULT(glClear, GL_COLOR_BUFFER_BIT);
        return true;
    };
    auto prof = nngn::Profile::context<nngn::Profile>(
        &nngn::Profile::stats.render);
    return render_pass();
}

bool OpenGLBackend::vsync() {
    NNGN_PROFILE_CONTEXT(vsync);
    glfwSwapBuffers(this->w);
    return true;
}

}

namespace nngn {

template<>
std::unique_ptr<Graphics> graphics_create_backend<backend>(const void *params) {
    return std::make_unique<OpenGLBackend>(
        *static_cast<const Graphics::OpenGLParameters*>(params), false);
}

template<>
std::unique_ptr<Graphics> graphics_create_backend<backend_es>(
    const void *params
) {
    return std::make_unique<OpenGLBackend>(
        *static_cast<const Graphics::OpenGLParameters*>(params), true);
}

}

#endif
