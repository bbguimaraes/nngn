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

#include <algorithm>
#include <array>
#include <cstring>
#include <list>
#include <string>
#include <string_view>
#include <vector>

#include "graphics/glfw.h"
#include "graphics/shaders.h"
#include "math/math.h"
#include "timing/profile.h"
#include "utils/flags.h"
#include "utils/literals.h"

#include "opengl.h"
#include "prog.h"
#include "resource.h"
#include "utils.h"
#include "vao.h"

using namespace std::string_literals;
using namespace std::string_view_literals;

using namespace nngn::literals;
using nngn::u8, nngn::i32, nngn::u32, nngn::u64;

namespace {

enum : u32 {
    CAMERA_UBO_BINDING,
};

constexpr auto N_PROGRAMS =
    static_cast<std::size_t>(nngn::Graphics::PipelineConfiguration::Type::MAX);

struct Pipeline {
    nngn::Graphics::PipelineConfiguration conf = {};
};

struct RenderList {
    struct Stage {
        struct Buffer {
            u32 vbo = {}, ebo = {};
            nngn::VAO vao = {};
        };
        explicit Stage(const nngn::Graphics::RenderList::Stage &s);
        u32 pipeline = {};
        std::vector<Buffer> buffers = {};
    };
    std::vector<Stage> normal = {};
};

class OpenGLBackend final : public nngn::GLFWBackend {
    enum Flag : u8 {
        ES = 1u << 0,
        CALLBACK_ERROR = 1u << 1,
        RESIZED = 1u << 2,
    };
    nngn::Flags<Flag> flags = {};
    int maj = 0, min = 0;
    nngn::GLBuffer camera_ubo = {};
    std::list<nngn::GLBuffer> stg_buffers = {};
    std::vector<nngn::GLBuffer> buffers = std::vector<nngn::GLBuffer>(1);
    std::vector<Pipeline> pipelines = {{}};
    std::array<nngn::GLProgram, N_PROGRAMS> programs = {};
    ::RenderList render_list = {};
    void resize(int, int) final { this->flags |= Flag::RESIZED; }
    static bool create_uniform_buffer(
        std::string_view name, u64 size, nngn::GLBuffer *b);
    bool create_vao(
        u32 vbo, u32 ebo, PipelineConfiguration::Type type, nngn::VAO *vao);
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
    bool set_n_frames(std::size_t) override { return true; }
    u32 create_pipeline(const PipelineConfiguration &conf) final;
    u32 create_buffer(const BufferConfiguration &conf) final;
    bool set_buffer_capacity(u32 b, u64 size) final;
    bool set_buffer_size(u32 b, u64 size) final;
    bool write_to_buffer(
        u32 b, u64 offset, u64 n, u64 size,
        void *data, void f(void*, void*, u64, u64)) final;
    bool set_render_list(const RenderList&) final;
    bool render() final;
    bool vsync() final;
};

OpenGLBackend::OpenGLBackend(const OpenGLParameters &p, bool es)
    : GLFWBackend(p), maj(p.maj), min(p.min)
    { if(es) flags.set(Flag::ES); }

RenderList::Stage::Stage(const nngn::Graphics::RenderList::Stage &s) :
    pipeline{s.pipeline}
{
    this->buffers.reserve(s.buffers.size());
    for(const auto &x : s.buffers)
        this->buffers.push_back({x.first, x.second, nngn::VAO{}});
}

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
    CHECK_RESULT(glEnable, GL_CULL_FACE);
    CHECK_RESULT(glEnable, GL_DEPTH_TEST);
    if(!OpenGLBackend::create_uniform_buffer("camera_ubo"sv,
            sizeof(nngn::CameraUBO), &this->camera_ubo))
        return false;
#define P(x) static_cast<std::size_t>(PipelineConfiguration::Type::x)
    nngn::GLProgram
        &triangle_prog = this->programs[P(TRIANGLE)];
#undef P
    if(!triangle_prog.create(
            "src/glsl/gl/triangle.vert"sv, "src/glsl/gl/triangle.frag"sv,
            nngn::GLSL_GL_TRIANGLE_VERT, nngn::GLSL_GL_TRIANGLE_FRAG))
        return false;
    CHECK_RESULT(glUseProgram, triangle_prog.id());
    CHECK_RESULT(
        glBindBufferRange, GL_UNIFORM_BUFFER,
        CAMERA_UBO_BINDING, this->camera_ubo.id(), 0, sizeof(nngn::CameraUBO));
    if(!triangle_prog.bind_ubo("Camera", CAMERA_UBO_BINDING))
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

bool OpenGLBackend::create_uniform_buffer(
    std::string_view name, u64 size, nngn::GLBuffer *b
) {
    return b->create(GL_UNIFORM_BUFFER, size, GL_DYNAMIC_DRAW)
        && nngn::gl_set_obj_name(NNGN_GL_BUFFER, b->id(), name);
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
    return true;
}

u32 OpenGLBackend::create_buffer(const BufferConfiguration &conf) {
    NNGN_LOG_CONTEXT_CF(OpenGLBackend);
    const auto ret = static_cast<u32>(this->buffers.size());
    const char *name = this->flags.is_set(Parameters::Flag::DEBUG)
        ? conf.name : nullptr;
    auto &b = this->buffers.emplace_back();
    if(!b.create(conf))
        return {};
    if(name && !nngn::gl_set_obj_name(NNGN_GL_BUFFER, b.id(), name))
        return {};
    return ret;
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
    nngn::GLBuffer stg;
    if(!stg.create(type, n * size, GL_STREAM_DRAW))
        return false;
    const auto isize = static_cast<GLsizeiptr>(n * size);
    void *const p = glMapBufferRange(type, 0, isize, access);
    if(!p)
        return nngn::gl_check_result("glMapBufferRange"), false;
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
    u32 vbo_idx, u32 ebo_idx, PipelineConfiguration::Type type, nngn::VAO *vao
) {
    NNGN_LOG_CONTEXT_CF(OpenGLBackend);
    using A = std::array<std::array<nngn::VAO::Attrib, 3>, N_PROGRAMS>;
    static constexpr A attrs = {{
        {{{"position", 3}, {"color", 3}}},
    }};
    static constexpr std::array names = {
        "triangle",
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
        && nngn::gl_set_obj_name(
            NNGN_GL_VERTEX_ARRAY, vao->id(), names[prog_idx]);
}

bool OpenGLBackend::set_buffer_capacity(u32 i, u64 size) {
    NNGN_LOG_CONTEXT_CF(OpenGLBackend);
    assert(i < this->buffers.size());
    return this->buffers[i].set_capacity(size);
}

bool OpenGLBackend::set_buffer_size(u32 i, u64 size) {
    NNGN_LOG_CONTEXT_CF(OpenGLBackend);
    assert(i < this->buffers.size());
    auto &b = this->buffers[i];
    const auto isize = static_cast<GLsizeiptr>(size);
    assert(isize <= b.capacity);
    if(b.size != isize)
        b.size = isize;
    return true;
}

bool OpenGLBackend::render() {
    NNGN_LOG_CONTEXT_CF(OpenGLBackend);
    if(this->flags.check_and_clear(Flag::RESIZED)) {
        int width = {}, height = {};
        glfwGetFramebufferSize(this->w, &width, &height);
        glViewport(0, 0, width, height);
        constexpr auto far = 2048.0f;
        const auto width_f = static_cast<float>(width) / 2.0f;
        const auto height_f = static_cast<float>(height) / 2.0f;
        const nngn::CameraUBO c = {
            .proj = nngn::Math::ortho(
                -width_f, width_f, -height_f, height_f, 0.0f, far),
            .view = nngn::Math::look_at<float>(
                    {0, 0, far / 2}, {}, {0, 1, 0})};
        CHECK_RESULT(glBindBuffer, GL_UNIFORM_BUFFER, this->camera_ubo.id());
        CHECK_RESULT(glBufferSubData, GL_UNIFORM_BUFFER, 0, sizeof(c), &c);
    }
    const auto render = [this](auto *l) {
        for(auto &x : *l) {
            assert(x.pipeline < this->pipelines.size());
            const auto &pipeline = this->pipelines[x.pipeline];
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
    const auto render_pass = [this, render] {
        nngn::GLDebugGroup group = {"color"sv};
        CHECK_RESULT(glClear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        return render(&render_list.normal);
    };
    auto prof = nngn::Profile::context<nngn::Profile>(
        &nngn::Profile::stats.render);
    if(!render_pass())
        return false;
    return LOG_RESULT(glBindVertexArray, 0);
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
