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

#include <algorithm>
#include <array>
#include <cstring>
#include <list>
#include <string>
#include <string_view>
#include <vector>

#ifdef NNGN_PLATFORM_EMSCRIPTEN
#define GLFW_INCLUDE_ES3
#else
#include <GL/glew.h>
#endif
#include <GLFW/glfw3.h>

#include "font/font.h"
#include "graphics/shaders.h"
#include "math/camera.h"
#include "math/math.h"
#include "timing/profile.h"
#include "utils/flags.h"

#include "glfw.h"

using namespace std::string_literals;
using namespace std::string_view_literals;

using nngn::u8, nngn::i32, nngn::u32, nngn::u64;

namespace {

enum : u32 {
    CAMERA_UBO_BINDING,
    MAIN_TEX_BINDING = 0,
    FONT_TEX_BINDING,
};

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

constexpr auto N_PROGRAMS =
    static_cast<std::size_t>(nngn::Graphics::PipelineConfiguration::Type::MAX);

template<typename T> class OpenGLHandle {
    u32 m_h = 0;
public:
    constexpr OpenGLHandle() = default;
    explicit constexpr OpenGLHandle(u32 h) noexcept : m_h(h) {}
    OpenGLHandle(const OpenGLHandle&) = delete;
    OpenGLHandle(OpenGLHandle &&lhs) noexcept
        { this->m_h = std::exchange(lhs.m_h, {}); }
    OpenGLHandle &operator=(const OpenGLHandle&) = delete;
    OpenGLHandle &operator=(OpenGLHandle &&lhs) noexcept
        { this->m_h = std::exchange(lhs.m_h, {}); }
    ~OpenGLHandle() { static_cast<T*>(this)->destroy(); }
    u32 id() const { return this->m_h; }
    u32 &id() { return this->m_h; }
};

struct GLShader final : OpenGLHandle<GLShader> {
    bool create(
        GLenum type, std::string_view name, std::span<const std::uint8_t> src);
    bool destroy();
};

struct GLBuffer final : OpenGLHandle<GLBuffer> {
    GLenum target = {}, usage = {};
    GLsizeiptr size = 0, capacity = 0;
    bool create(GLenum target, u64 size, GLenum usage);
    bool create(const nngn::Graphics::BufferConfiguration &conf);
    bool set_capacity(u64 n);
    bool destroy();
};

struct GLProgram : OpenGLHandle<GLProgram> {
    bool create(u32 vert, u32 frag);
    bool destroy();
    bool get_uniform_location(const char *name, int *l) const;
    bool bind_ubo(const char *name, u32 binding) const;
};

struct VAO final : OpenGLHandle<VAO> {
    struct Attrib { std::string_view name; GLsizei size; };
    u32 vbo = {}, ebo = {};
    bool create(u32 vbo, u32 ebo);
    bool destroy();
    bool vertex_attrib_pointers(
        const GLProgram &prog, std::size_t n, const Attrib *p);
};

struct GLTexArray : OpenGLHandle<GLTexArray> {
    bool create(
        GLenum type, GLenum fmt, GLint wrap,
        const nngn::ivec3 &extent, GLsizei mip_levels);
    bool destroy();
};

struct Pipeline {
    nngn::Graphics::PipelineConfiguration conf = {};
};

struct RenderList {
    struct Stage {
        struct Buffer {
            u32 vbo = {}, ebo = {};
            VAO vao = {};
        };
        explicit Stage(const nngn::Graphics::RenderList::Stage &s);
        u32 pipeline = {};
        std::vector<Buffer> buffers = {};
    };
    std::vector<Stage> normal = {}, overlay = {}, hud = {};
};

class OpenGLBackend final : public nngn::GLFWBackend {
    enum Flag : u8 {
        ES = 1u << 0, CALLBACK_ERROR = 1u << 1, RESIZED = 1u << 2,
        CAMERA_UPDATED = 1u << 3,
    };
    nngn::Flags<Flag> flags = {};
    int maj = 0, min = 0;
    GLBuffer camera_ubo = {}, camera_hud_ubo = {};
    std::list<GLBuffer> stg_buffers = {};
    std::vector<GLBuffer> buffers = std::vector<GLBuffer>(1);
    std::vector<Pipeline> pipelines = {{}};
    std::array<GLProgram, N_PROGRAMS> programs = {};
    GLint triangle_prog_alpha_loc = -1;
    ::RenderList render_list = {};
    GLTexArray tex = {}, font_tex = {};
    void resize(int, int) final
        { this->flags |= Flag::RESIZED | Flag::CAMERA_UPDATED; }
    static bool set_obj_name(GLenum type, GLuint obj, std::string_view name);
    static bool create_tex_array(
        std::string_view name,
        GLenum type, GLenum fmt, GLint wrap,
        const nngn::ivec3 &extent, GLsizei mip_levels,
        GLTexArray *t);
    static bool create_uniform_buffer(
        std::string_view name, u64 size, GLBuffer *b);
    static bool create_prog(
        std::string_view vs_name, std::string_view fs_name,
        std::span<const std::uint8_t> vs, std::span<const std::uint8_t> fs,
        GLProgram *prog);
    bool create_vao(
        u32 vbo, u32 ebo, PipelineConfiguration::Type type, VAO *vao);
public:
    OpenGLBackend(const OpenGLParameters &params, bool es);
    Version version() const final;
    bool init_backend() final;
    bool init_instance() final;
    bool init_device() final { return true; }
    bool init_device(std::size_t) final { return true; }
    std::size_t n_extensions() const final;
    std::size_t n_layers() const final { return 0; }
    std::size_t n_devices() const final { return 1; }
    std::size_t n_device_extensions(std::size_t) const final { return 0; }
    std::size_t n_queue_families(std::size_t) const final { return 0; }
    std::size_t n_present_modes() const final { return 0; }
    std::size_t n_heaps(std::size_t) const final { return 0; }
    std::size_t n_memory_types(std::size_t, std::size_t) const final
        { return 0; }
    std::size_t selected_device() const final { return 0; }
    void extensions(Extension*) const final;
    void layers(Layer*) const final {}
    void device_infos(DeviceInfo*) const final;
    void device_extensions(std::size_t, Extension*) const final {}
    void queue_families(std::size_t, QueueFamily*) const final {}
    SurfaceInfo surface_info() const final { return {}; }
    void present_modes(PresentMode*) const final {}
    void heaps(std::size_t, MemoryHeap*) const final {}
    void memory_types(std::size_t, std::size_t, MemoryType*) const final {}
    bool error() final { return this->flags.is_set(Flag::CALLBACK_ERROR); }
    nngn::GraphicsStats stats() override { return {}; }
    void set_camera_updated() final { this->flags |= Flag::CAMERA_UPDATED; }
    bool set_n_frames(std::size_t) override { return true; }
    u32 create_pipeline(const PipelineConfiguration &conf) final;
    u32 create_buffer(const BufferConfiguration &conf) final;
    bool set_buffer_capacity(u32 b, u64 size) final;
    void set_buffer_size(u32 b, u64 size) final;
    bool write_to_buffer(
        u32 b, u64 offset, u64 n, u64 size,
        void *data, void f(void*, void*, u64, u64)) final;
    bool resize_textures(u32 s) final;
    bool load_textures(u32 i, u32 n, const std::byte *v) final;
    bool resize_font(u32 s) final;
    bool load_font(
        unsigned char c, u32 n,
        const nngn::uvec2 *size, const std::byte *v) final;
    bool set_render_list(const RenderList&) final;
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

bool GLShader::create(
    GLenum type, std::string_view name, std::span<const std::uint8_t> src
) {
    NNGN_LOG_CONTEXT_CF(GLShader);
    NNGN_LOG_CONTEXT(name.data());
    if(!(this->id() = glCreateShader(type))) {
        check_result("glCreateShader");
        return false;
    }
    const auto *const data_p = static_cast<const GLchar*>(
        static_cast<const void*>(src.data()));
    const auto size_p = static_cast<GLint>(src.size());
    CHECK_RESULT(glShaderSource, this->id(), 1, &data_p, &size_p);
    CHECK_RESULT(glCompileShader, this->id());
    i32 status = {};
    CHECK_RESULT(glGetShaderiv, this->id(), GL_COMPILE_STATUS, &status);
    if(!status) {
        i32 size = {};
        CHECK_RESULT(glGetShaderiv, this->id(), GL_INFO_LOG_LENGTH, &size);
        std::vector<char> log(static_cast<size_t>(size));
        CHECK_RESULT(glGetShaderInfoLog,
            this->id(), size, nullptr, log.data());
        nngn::Log::l() << "glCompileShader:\n" << log.data();
        return false;
    }
    return true;
}

bool GLShader::destroy() {
    NNGN_LOG_CONTEXT_CF(GLShader);
    if(this->id())
        CHECK_RESULT(glDeleteShader, this->id());
    return true;
}

bool GLProgram::create(u32 vert, u32 frag) {
    NNGN_LOG_CONTEXT_CF(GLProgram);
    if(!(this->id() = glCreateProgram())) {
        check_result("glCreateProgram");
        return false;
    }
    CHECK_RESULT(glAttachShader, this->id(), vert);
    CHECK_RESULT(glAttachShader, this->id(), frag);
    CHECK_RESULT(glLinkProgram, this->id());
    i32 status = {};
    CHECK_RESULT(glGetProgramiv, this->id(), GL_LINK_STATUS, &status);
    if(!status) {
        i32 size = {};
        CHECK_RESULT(glGetProgramiv, this->id(), GL_INFO_LOG_LENGTH, &size);
        std::vector<char> log(static_cast<size_t>(size));
        CHECK_RESULT(glGetProgramInfoLog,
            this->id(), size, nullptr, log.data());
        nngn::Log::l() << "glLinkProgram:\n" << log.data();
        return false;
    }
    return true;
}

bool GLProgram::destroy() {
    if(this->id())
        CHECK_RESULT(glDeleteProgram, this->id());
    return true;
}

bool GLProgram::get_uniform_location(const char *name, int *l) const {
    if((*l = glGetUniformLocation(this->id(), name)) != -1)
        return true;
    nngn::Log::l() << "glGetUniformLocation(" << name << "): -1" << std::endl;
    check_result("glGetUniformLocation");
    return false;
}

bool GLProgram::bind_ubo(const char *name, u32 binding) const {
    if(const auto i = glGetUniformBlockIndex(this->id(), name);
            i != GL_INVALID_INDEX) {
        CHECK_RESULT(glUniformBlockBinding, this->id(), i, binding);
        return true;
    }
    check_result("glGetUniformBlockIndex");
    nngn::Log::l() << "glGetUniformBlockIndex: GL_INVALID_INDEX" << std::endl;
    return false;
}

bool VAO::create(u32 vbo_, u32 ebo_) {
    NNGN_LOG_CONTEXT_CF(VAO);
    CHECK_RESULT(glGenVertexArrays, 1, &this->id());
    this->vbo = vbo_;
    this->ebo = ebo_;
    return true;
}

bool VAO::destroy() {
    NNGN_LOG_CONTEXT_CF(VAO);
    if(this->id())
        CHECK_RESULT(glDeleteVertexArrays, 1, &this->id());
    return true;
}

bool VAO::vertex_attrib_pointers(
    const GLProgram &prog, size_t n, const Attrib *p
) {
    NNGN_LOG_CONTEXT_CF(VAO);
    GLsizei stride = 0;
    for(size_t i = 0; i < n; ++i)
        stride += p[i].size;
    CHECK_RESULT(glBindVertexArray, this->id());
    CHECK_RESULT(glBindBuffer, GL_ARRAY_BUFFER, this->vbo);
    CHECK_RESULT(glBindBuffer, GL_ELEMENT_ARRAY_BUFFER, this->ebo);
    GLsizei offset = 0;
    for(size_t i = 0; i < n; ++i) {
        const auto &a = p[i];
        GLint l = -1;
        if(!a.name.empty()) {
            if((l = glGetAttribLocation(prog.id(), a.name.data())) == -1) {
                check_result("glGetAttribLocation");
                nngn::Log::l() << "glGetAttribLocation: " << a.name << ": -1\n";
                return false;
            }
            const auto ul = static_cast<GLuint>(l);
            constexpr auto si = static_cast<GLsizei>(sizeof(float));
            CHECK_RESULT(glVertexAttribPointer,
                ul, a.size, GL_FLOAT, GL_FALSE,
                stride * si, reinterpret_cast<void*>(offset * si));
            CHECK_RESULT(glEnableVertexAttribArray, ul);
        }
        offset += a.size;
    }
    return true;
}

bool GLBuffer::create(GLenum target_, u64 size_, GLenum usage_) {
    NNGN_LOG_CONTEXT_CF(GLBuffer);
    CHECK_RESULT(glGenBuffers, 1, &this->id());
    CHECK_RESULT(glBindBuffer, target_, this->id());
    const auto cap = static_cast<GLsizeiptr>(size_);
    this->target = target_;
    this->capacity = cap;
    this->usage = usage_;
    return !size_ || this->set_capacity(size_);
}

bool GLBuffer::set_capacity(u64 n) {
    NNGN_LOG_CONTEXT_CF(GLBuffer);
    assert(this->id());
    const auto cap = static_cast<GLsizeiptr>(n);
    CHECK_RESULT(glBindBuffer, this->target, this->id());
    CHECK_RESULT(glBufferData, this->target, cap, nullptr, this->usage);
    this->capacity = cap;
    return true;
}

bool GLBuffer::create(const nngn::Graphics::BufferConfiguration &conf) {
    return this->create(
        conf.type == nngn::Graphics::BufferConfiguration::Type::VERTEX
            ? GL_ARRAY_BUFFER : GL_ELEMENT_ARRAY_BUFFER,
        conf.size,
        GL_DYNAMIC_DRAW);
}

bool GLBuffer::destroy() {
    NNGN_LOG_CONTEXT_CF(GLBuffer);
    if(this->id())
        CHECK_RESULT(glDeleteBuffers, 1, &this->id());
    return true;
}

bool GLTexArray::create(
        GLenum type, GLenum fmt, GLint wrap,
        const nngn::ivec3 &extent, GLsizei mip_levels) {
    CHECK_RESULT(glBindTexture, type, this->id());
    CHECK_RESULT(glTexStorage3D,
        type, mip_levels, fmt, extent.x, extent.y, extent.z);
    CHECK_RESULT(glTexParameteri, type, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    CHECK_RESULT(glTexParameteri, type, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    CHECK_RESULT(glTexParameteri, type, GL_TEXTURE_WRAP_R, wrap);
    CHECK_RESULT(glTexParameteri, type, GL_TEXTURE_WRAP_S, wrap);
    CHECK_RESULT(glTexParameteri, type, GL_TEXTURE_WRAP_T, wrap);
    if(mip_levels > 1)
        CHECK_RESULT(glGenerateMipmap, type);
    return true;
}

bool GLTexArray::destroy() {
    NNGN_LOG_CONTEXT_CF(GLTexArray);
    if(!this->id())
        return true;
    CHECK_RESULT(glDeleteTextures, 1, &this->id());
    this->id() = 0;
    return true;
}

RenderList::Stage::Stage(const nngn::Graphics::RenderList::Stage &s) {
    this->pipeline = s.pipeline;
    this->buffers.reserve(s.buffers.size());
    for(const auto &x : s.buffers)
        this->buffers.push_back({x.first, x.second, VAO{}});
}

#ifdef GL_VERSION_4_3
bool OpenGLBackend::set_obj_name(
        GLenum type, GLuint obj, std::string_view name) {
    CHECK_RESULT(glObjectLabel,
        type, obj, static_cast<GLsizei>(name.size()), name.data());
    return true;
}
#else
bool OpenGLBackend::set_obj_name(GLenum, GLuint, std::string_view)
    { return true; }
#endif

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

bool OpenGLBackend::init_backend() {
    NNGN_LOG_CONTEXT_CF(OpenGLBackend);
    return this->init_glfw();
}

bool OpenGLBackend::init_instance() {
    NNGN_LOG_CONTEXT_CF(OpenGLBackend);
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
    CHECK_RESULT(glEnable, GL_BLEND);
    CHECK_RESULT(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    CHECK_RESULT(glGenTextures, 1, &this->tex.id());
    if(!OpenGLBackend::create_uniform_buffer("camera_ubo"sv,
            sizeof(nngn::CameraUBO), &this->camera_ubo))
        return false;
    if(!OpenGLBackend::create_uniform_buffer("camera_hud_ubo"sv,
            sizeof(nngn::CameraUBO), &this->camera_hud_ubo))
        return false;
#define P(x) static_cast<std::size_t>(PipelineConfiguration::Type::x)
    GLProgram
        &triangle_prog = this->programs[P(TRIANGLE)],
        &sprite_prog = this->programs[P(SPRITE)],
        &voxel_prog = this->programs[P(VOXEL)],
        &font_prog = this->programs[P(FONT)];
#undef P
    if(!OpenGLBackend::create_prog(
            "src/glsl/gl/triangle.vert"sv,
            "src/glsl/gl/triangle.frag"sv,
            nngn::GLSL_GL_TRIANGLE_VERT,
            nngn::GLSL_GL_TRIANGLE_FRAG,
            &triangle_prog))
        return false;
    CHECK_RESULT(glUseProgram, triangle_prog.id());
    if(!triangle_prog.get_uniform_location("alpha", &triangle_prog_alpha_loc))
        return false;
    CHECK_RESULT(glUniform1f, triangle_prog_alpha_loc, 1);
    if(!triangle_prog.bind_ubo("Camera", CAMERA_UBO_BINDING))
        return false;
    if(!OpenGLBackend::create_prog(
            "src/glsl/gl/sprite.vert"sv,
            "src/glsl/gl/sprite.frag"sv,
            nngn::GLSL_GL_SPRITE_VERT,
            nngn::GLSL_GL_SPRITE_FRAG,
            &sprite_prog))
        return false;
    CHECK_RESULT(glUseProgram, sprite_prog.id());
    if(!sprite_prog.bind_ubo("Camera", CAMERA_UBO_BINDING))
        return false;
    if(!OpenGLBackend::create_prog(
            "src/glsl/gl/sprite.vert"sv,
            "src/glsl/gl/voxel.frag"sv,
            nngn::GLSL_GL_SPRITE_VERT,
            nngn::GLSL_GL_VOXEL_FRAG,
            &voxel_prog))
        return false;
    CHECK_RESULT(glUseProgram, voxel_prog.id());
    if(!voxel_prog.bind_ubo("Camera", CAMERA_UBO_BINDING))
        return false;
    if(!OpenGLBackend::create_prog(
            "src/glsl/gl/font.vert"sv,
            "src/glsl/gl/font.frag"sv,
            nngn::GLSL_GL_FONT_VERT,
            nngn::GLSL_GL_FONT_FRAG,
            &font_prog))
        return false;
    CHECK_RESULT(glUseProgram, font_prog.id());
    if(!font_prog.bind_ubo("Camera", CAMERA_UBO_BINDING))
        return false;
    if(!this->params.flags.is_set(Parameters::Flag::HIDDEN))
        glfwShowWindow(this->w);
    return true;
}

std::size_t OpenGLBackend::n_extensions() const {
    GLint ret = {};
    CHECK_RESULT(glGetIntegerv, GL_NUM_EXTENSIONS, &ret);
    return static_cast<std::size_t>(ret);
}

void OpenGLBackend::extensions(Extension *p) const {
    constexpr auto size = std::tuple_size<decltype(Extension::name)>::value - 1;
    const auto n = this->n_extensions();
    for(std::size_t i = 0; i < n; ++i) {
        const auto *name = reinterpret_cast<const char*>(
            glGetStringi(GL_EXTENSIONS, static_cast<GLuint>(i)));
        std::strncpy(p[i].name.data(), name, size);
    }
}

void OpenGLBackend::device_infos(DeviceInfo *p) const {
    constexpr auto size =
        std::tuple_size<decltype(DeviceInfo::name)>::value - 1;
    const auto *name = reinterpret_cast<const char*>(
        glGetString(GL_RENDERER));
    std::strncpy(p->name.data(), name, size);
}

bool OpenGLBackend::create_tex_array(
        std::string_view name,
        GLenum type, GLenum fmt, GLint wrap,
        const nngn::ivec3 &extent, GLsizei mip_levels, GLTexArray *t) {
    NNGN_LOG_CONTEXT_CF(OpenGLBackend);
    NNGN_LOG_CONTEXT(name.data());
    return t->create(type, fmt, wrap, extent, mip_levels)
        && OpenGLBackend::set_obj_name(GL_TEXTURE, t->id(), name);
}

bool OpenGLBackend::create_uniform_buffer(
    std::string_view name, u64 size, GLBuffer *b
) {
    return b->create(GL_UNIFORM_BUFFER, size, GL_DYNAMIC_DRAW)
        && OpenGLBackend::set_obj_name(NNGN_GL_BUFFER, b->id(), name);
}

bool OpenGLBackend::create_prog(
    std::string_view vs_name, std::string_view fs_name,
    std::span<const std::uint8_t> vs, std::span<const std::uint8_t> fs,
    GLProgram *prog
) {
    NNGN_LOG_CONTEXT_CF(OpenGLBackend);
    NNGN_LOG_CONTEXT(vs_name.data());
    NNGN_LOG_CONTEXT(fs_name.data());
    GLShader v, f;
    return v.create(GL_VERTEX_SHADER, vs_name, vs)
        && OpenGLBackend::set_obj_name(NNGN_GL_SHADER, v.id(), vs_name)
        && f.create(GL_FRAGMENT_SHADER, fs_name, fs)
        && OpenGLBackend::set_obj_name(NNGN_GL_SHADER, f.id(), fs_name)
        && prog->create(v.id(), f.id())
        && OpenGLBackend::set_obj_name(
            NNGN_GL_PROGRAM, prog->id(),
            vs_name.data() + "+"s + fs_name.data());
}

u32 OpenGLBackend::create_pipeline(const PipelineConfiguration &conf) {
    const auto ret = static_cast<u32>(this->pipelines.size());
    this->pipelines.push_back({conf});
    return ret;
}

bool OpenGLBackend::set_render_list(const RenderList &l) {
    NNGN_LOG_CONTEXT_CF(OpenGLBackend);
    constexpr auto f = [](auto *dst, const auto &src) {
        dst->reserve(src.size());
        for(const auto &s : src)
            dst->emplace_back(s);
    };
    f(&this->render_list.normal, l.normal);
    f(&this->render_list.overlay, l.overlay);
    f(&this->render_list.hud, l.hud);
    return true;
}

u32 OpenGLBackend::create_buffer(const BufferConfiguration &conf) {
    NNGN_LOG_CONTEXT_CF(OpenGLBackend);
    const auto ret = static_cast<u32>(this->buffers.size());
    const char *name = this->flags.is_set(Parameters::Flag::DEBUG)
        ? conf.name : nullptr;
    auto &b = this->buffers.emplace_back();
    return (
        b.create(conf) && (!name || this->set_obj_name(GL_BUFFER, b.id(), name))
    ) ? ret : u32{};
}

bool OpenGLBackend::write_to_buffer(
    u32 i, u64 offset, u64 n, u64 size,
    void *data, void f(void*, void*, u64, u64)
) {
    NNGN_LOG_CONTEXT_CF(OpenGLBackend);
    constexpr auto access = GL_MAP_WRITE_BIT
        | (nngn::Platform::emscripten * GL_MAP_INVALIDATE_BUFFER_BIT);
    assert(i);
    assert(i < this->buffers.size());
    const auto &b = this->buffers[i];
    const GLenum type = b.target == GL_ARRAY_BUFFER
        ? GL_COPY_READ_BUFFER
        : GL_ELEMENT_ARRAY_BUFFER;
    GLBuffer stg;
    if(!stg.create(type, n * size, GL_STREAM_DRAW))
        return false;
    const auto isize = static_cast<GLsizeiptr>(n * size);
    void *const p = glMapBufferRange(type, 0, isize, access);
    if(!p)
        return check_result("glMapBufferRange"), false;
    f(data, p, 0, n);
    CHECK_RESULT(glUnmapBuffer, type);
    CHECK_RESULT(glBindBuffer, GL_COPY_WRITE_BUFFER, b.id());
    CHECK_RESULT(glBindBuffer, GL_COPY_READ_BUFFER, stg.id());
    CHECK_RESULT(glCopyBufferSubData,
        GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER,
        0, static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(n * size));
    return true;
}

bool OpenGLBackend::create_vao(
    u32 vbo_idx, u32 ebo_idx, PipelineConfiguration::Type type, VAO *vao
) {
    NNGN_LOG_CONTEXT_CF(OpenGLBackend);
    using A = std::array<std::array<VAO::Attrib, 3>, N_PROGRAMS>;
    static constexpr A attrs = {{
        {{{"position", 3}, {"color", 3}}},
        {{{"position", 3}, {"tex_coord", 3}}},
        {{{"position", 3}, {"tex_coord", 3}}},
        {{{"position", 3}, {"tex_coord", 3}}},
    }};
    static constexpr std::array names = {
        "triangle", "sprite", "voxel", "font",
    };
    assert(vbo_idx < this->buffers.size());
    assert(ebo_idx < this->buffers.size());
    auto &vbo = this->buffers[vbo_idx], &ebo = this->buffers[ebo_idx];
    if(!vbo.id() || !ebo.id())
        return true;
    const auto prog_idx = static_cast<std::size_t>(type);
    const auto &prog = this->programs[prog_idx];
    CHECK_RESULT(glUseProgram, prog.id());
    const auto &attr = attrs[prog_idx];
    return vao->create(vbo.id(), ebo.id())
        && vao->vertex_attrib_pointers(prog, attr.size(), attr.data())
        && OpenGLBackend::set_obj_name(
            NNGN_GL_VERTEX_ARRAY, vao->id(), names[prog_idx]);
}

bool OpenGLBackend::set_buffer_capacity(u32 i, u64 size) {
    NNGN_LOG_CONTEXT_CF(OpenGLBackend);
    assert(i < this->buffers.size());
    return this->buffers[i].set_capacity(size);
}

void OpenGLBackend::set_buffer_size(u32 i, u64 size) {
    NNGN_LOG_CONTEXT_CF(OpenGLBackend);
    assert(i < this->buffers.size());
    auto &b = this->buffers[i];
    const auto isize = static_cast<GLsizeiptr>(size);
    assert(isize <= b.capacity);
    if(b.size != isize)
        b.size = isize;
}

bool OpenGLBackend::resize_textures(u32 s) {
    CHECK_RESULT(glActiveTexture, GL_TEXTURE0 + MAIN_TEX_BINDING);
    const auto si = static_cast<GLint>(s);
    return OpenGLBackend::create_tex_array("tex"sv,
        GL_TEXTURE_2D_ARRAY, GL_RGBA8, GL_REPEAT,
        {Graphics::TEXTURE_EXTENT, Graphics::TEXTURE_EXTENT, si},
        static_cast<GLsizei>(Graphics::TEXTURE_MIP_LEVELS),
        &this->tex);
}

bool OpenGLBackend::load_textures(u32 i, u32 n, const std::byte *v) {
    NNGN_LOG_CONTEXT_CF(OpenGLBackend);
    const u32 base_layer = i;
    constexpr auto type = GL_TEXTURE_2D_ARRAY;
    CHECK_RESULT(glActiveTexture, GL_TEXTURE0 + MAIN_TEX_BINDING);
    CHECK_RESULT(glBindTexture, type, this->tex.id());
    for(i = 0; i < n; ++i)
        CHECK_RESULT(
            glTexSubImage3D, type, 0, 0, 0, static_cast<GLint>(base_layer + i),
            Graphics::TEXTURE_EXTENT, Graphics::TEXTURE_EXTENT, 1, GL_RGBA,
            GL_UNSIGNED_BYTE, v + i * Graphics::TEXTURE_SIZE);
    CHECK_RESULT(glGenerateMipmap, type);
    return true;
}

bool OpenGLBackend::resize_font(uint32_t s) {
    this->font_tex.destroy();
    CHECK_RESULT(glActiveTexture, GL_TEXTURE0 + FONT_TEX_BINDING);
    CHECK_RESULT(glGenTextures, 1, &this->font_tex.id());
    const auto si = static_cast<int32_t>(s);
    return this->create_tex_array("font_tex"sv,
        GL_TEXTURE_2D_ARRAY, GL_RGBA8, GL_REPEAT, {si, si, nngn::Font::N},
        static_cast<GLsizei>(nngn::Math::mip_levels(s)),
        &this->font_tex);
}

bool OpenGLBackend::load_font(
    unsigned char c, u32 n, const nngn::uvec2 *size, const std::byte *v
) {
    NNGN_LOG_CONTEXT_CF(OpenGLBackend);
    constexpr auto type = GL_TEXTURE_2D_ARRAY;
    CHECK_RESULT(glActiveTexture, GL_TEXTURE0 + FONT_TEX_BINDING);
    CHECK_RESULT(glBindTexture, type, this->font_tex.id());
    for(size_t i = 0; i < n; ++i, v += 4 * size[c].x * size[c].y)
        CHECK_RESULT(
            glTexSubImage3D, type, 0, 0, 0, static_cast<GLint>(c + i),
            static_cast<GLsizei>(size[i].x), static_cast<GLsizei>(size[i].y),
            1, GL_RGBA, GL_UNSIGNED_BYTE, v);
    CHECK_RESULT(glGenerateMipmap, type);
    return true;
}

bool OpenGLBackend::render() {
    NNGN_LOG_CONTEXT_CF(OpenGLBackend);
    auto prof = nngn::Profile::context<nngn::Profile>(
        &nngn::Profile::stats.render);
    if(this->flags & Flag::RESIZED) {
        this->flags.clear(Flag::RESIZED);
        int width = {}, height = {};
        glfwGetFramebufferSize(this->w, &width, &height);
        glViewport(0, 0, width, height);
    }
    const auto update_camera = [this]() {
        if(~this->flags & Flag::CAMERA_UPDATED)
            return true;
        this->flags.clear(Flag::CAMERA_UPDATED);
        nngn::CameraUBO c = {.proj = *this->camera.proj * *this->camera.view};
        CHECK_RESULT(glBindBuffer, GL_UNIFORM_BUFFER, this->camera_ubo.id());
        CHECK_RESULT(glBufferSubData, GL_UNIFORM_BUFFER, 0, sizeof(c), &c);
        c = {*this->camera.hud_proj, nngn::mat4(1)};
        CHECK_RESULT(glBindBuffer,
            GL_UNIFORM_BUFFER, this->camera_hud_ubo.id());
        CHECK_RESULT(glBufferSubData, GL_UNIFORM_BUFFER, 0, sizeof(c), &c);
        return true;
    };
    const auto render = [this](auto *l) {
        using PFlag = PipelineConfiguration::Flag;
        for(auto &x : *l) {
            assert(x.pipeline < this->pipelines.size());
            const auto &pipeline = this->pipelines[x.pipeline];
            if(pipeline.conf.flags & PFlag::DEPTH_TEST)
                CHECK_RESULT(glEnable, GL_DEPTH_TEST);
            else
                CHECK_RESULT(glDisable, GL_DEPTH_TEST);
            if(pipeline.conf.flags & PFlag::CULL_BACK_FACES)
                CHECK_RESULT(glEnable, GL_CULL_FACE);
            else
                CHECK_RESULT(glDisable, GL_CULL_FACE);
            const auto type = pipeline.conf.type;
            const auto &prog = this->programs[static_cast<std::size_t>(type)];
            CHECK_RESULT(glUseProgram, prog.id());
            for(auto &[vbo_idx, ebo_idx, vao] : x.buffers) {
                if(!vao.id() && !this->create_vao(vbo_idx, ebo_idx, type, &vao))
                    return false;
                if(!vao.ebo)
                    continue;
                assert(ebo_idx < this->buffers.size());
                const auto &ebo = this->buffers[ebo_idx];
                if(!ebo.size)
                    continue;
                CHECK_RESULT(glBindVertexArray, vao.id());
                CHECK_RESULT(glDrawElements,
                    GL_TRIANGLES,
                    static_cast<GLsizei>(
                        ebo.size / static_cast<GLsizeiptr>(sizeof(u32))),
                    GL_UNSIGNED_INT, nullptr);
            }
        }
        return true;
    };
    if(!update_camera())
        return false;
    CHECK_RESULT(
        glBindBufferRange, GL_UNIFORM_BUFFER,
        CAMERA_UBO_BINDING, this->camera_ubo.id(), 0, sizeof(nngn::CameraUBO));
    CHECK_RESULT(glClear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    const auto &triangle_prog = this->programs[
        static_cast<std::size_t>(PipelineConfiguration::Type::TRIANGLE)];
    CHECK_RESULT(glUseProgram, triangle_prog.id());
    CHECK_RESULT(glUniform1f, triangle_prog_alpha_loc, 1);
    if(!render(&render_list.normal))
        return false;
    CHECK_RESULT(glUseProgram, triangle_prog.id());
    CHECK_RESULT(glUniform1f, triangle_prog_alpha_loc, .5);
    if(!render(&render_list.overlay))
        return false;
    CHECK_RESULT(
        glBindBufferRange, GL_UNIFORM_BUFFER,
        CAMERA_UBO_BINDING, this->camera_hud_ubo.id(), 0,
        sizeof(nngn::CameraUBO));
    CHECK_RESULT(glActiveTexture, GL_TEXTURE0 + MAIN_TEX_BINDING);
    CHECK_RESULT(glBindTexture, GL_TEXTURE_2D_ARRAY, this->font_tex.id());
    if(!render(&render_list.hud))
        return false;
    CHECK_RESULT(glBindTexture, GL_TEXTURE_2D_ARRAY, this->tex.id());
    CHECK_RESULT(glBindVertexArray, 0);
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
