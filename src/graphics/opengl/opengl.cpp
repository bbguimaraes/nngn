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
#include <numeric>
#include <string>
#include <string_view>
#include <vector>

#include "font/font.h"
#include "graphics/glfw.h"
#include "graphics/shaders.h"
#include "math/camera.h"
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
    LIGHTS_UBO_BINDING,
    CAMERA_UBO_BINDING,
    MAIN_TEX_BINDING = 0,
    FONT_TEX_BINDING,
    SHADOW_MAP_TEX_BINDING,
    SHADOW_CUBE_TEX_BINDING,
};

constexpr auto SHADOW_TEX_FORMAT = GL_DEPTH_COMPONENT32F;

constexpr auto N_PROGRAMS =
    static_cast<std::size_t>(nngn::Graphics::PipelineConfiguration::Type::MAX);

struct GLShadowMap : public nngn::GLTexArray {
    bool init(u32 size);
};

struct GLShadowCube : public nngn::GLTexArray {
    bool init(u32 size);
};

struct GLFramebuffer : nngn::OpenGLHandle<GLFramebuffer> {
    bool create();
    bool destroy();
};

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
    std::vector<Stage>
        depth = {}, map_ortho = {}, map_persp = {},
        normal = {}, overlay = {}, screen = {},
        shadow_maps = {}, shadow_cubes = {};
};

class OpenGLBackend final : public nngn::GLFWBackend {
    enum Flag : u8 {
        ES = 1u << 0,
        CALLBACK_ERROR = 1u << 1,
        CAMERA_UPDATED = 1u << 2,
        LIGHTING_UPDATED = 1u << 3,
    };
    nngn::Flags<Flag> flags = {};
    int maj = 0, min = 0;
    GLsizei shadow_map_size = SHADOW_MAP_INITIAL_SIZE;
    GLsizei shadow_cube_size = SHADOW_CUBE_INITIAL_SIZE;
    nngn::GLBuffer camera_ubo = {}, camera_screen_ubo = {};
    std::array<nngn::GLBuffer, 7_z * NNGN_MAX_LIGHTS>
        shadow_camera_ubos = {};
    nngn::GLBuffer lights_ubo = {}, no_lights_ubo = {};
    std::list<nngn::GLBuffer> stg_buffers = {};
    std::vector<nngn::GLBuffer> buffers = std::vector<nngn::GLBuffer>(1);
    std::vector<Pipeline> pipelines = {{}};
    std::array<nngn::GLProgram, N_PROGRAMS> programs = {};
    GLint triangle_prog_alpha_loc = -1;
    ::RenderList render_list = {};
    nngn::GLTexArray tex = {}, font_tex = {};
    GLFramebuffer lights_fb = {};
    GLShadowMap shadow_map = {};
    std::array<GLShadowCube, NNGN_MAX_LIGHTS> shadow_cubes = {};
    void resize(int, int) final { this->flags |= Flag::CAMERA_UPDATED; }
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
    bool set_n_frames(std::size_t) final { return true; }
    bool set_n_swap_chain_images(std::size_t) final { return true; }
    nngn::GraphicsStats stats() final { return {}; }
    void set_camera_updated() final { this->flags |= Flag::CAMERA_UPDATED; }
    void set_lighting_updated() final
        { this->flags |= Flag::LIGHTING_UPDATED; }
    bool set_shadow_map_size(u32 s) final;
    bool set_shadow_cube_size(u32 s) final;
    u32 create_pipeline(const PipelineConfiguration &conf) final;
    u32 create_buffer(const BufferConfiguration &conf) final;
    bool set_buffer_capacity(u32 b, u64 size) final;
    bool set_buffer_size(u32 b, u64 size) final;
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

OpenGLBackend::OpenGLBackend(const OpenGLParameters &p, bool es)
    : GLFWBackend(p), maj(p.maj), min(p.min)
    { if(es) flags.set(Flag::ES); }

bool GLShadowMap::init(u32 size) {
    const auto si = static_cast<i32>(size);
    return nngn::GLTexArray::create(
        GL_TEXTURE_2D_ARRAY, SHADOW_TEX_FORMAT, GL_NEAREST, GL_NEAREST,
#ifdef GL_VERSION_2
        GL_CLAMP_TO_BORDER,
#else
        GL_CLAMP_TO_EDGE,
#endif
        {si, si, NNGN_MAX_LIGHTS}, 1);
}

bool GLShadowCube::init(u32 size) {
    const auto si = static_cast<i32>(size);
    return nngn::GLTexArray::create(
        GL_TEXTURE_CUBE_MAP, SHADOW_TEX_FORMAT, GL_NEAREST, GL_NEAREST,
        GL_CLAMP_TO_EDGE, {si, si, 1}, 1);
}

bool GLFramebuffer::destroy() {
    if(this->id())
        CHECK_RESULT(glDeleteFramebuffers, 1, &this->id());
    return true;
}

bool GLFramebuffer::create() {
    CHECK_RESULT(glGenFramebuffers, 1, &this->id());
    CHECK_RESULT(glBindFramebuffer, GL_FRAMEBUFFER, this->id());
    CHECK_RESULT(glDrawBuffers, 0, nullptr);
    CHECK_RESULT(glReadBuffer, GL_NONE);
    return true;
}

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
    CHECK_RESULT(glEnable, GL_BLEND);
    CHECK_RESULT(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if(!this->lights_fb.create())
        return false;
    CHECK_RESULT(glBindFramebuffer, GL_FRAMEBUFFER, 0);
    CHECK_RESULT(glGenTextures, 1, &this->tex.id());
    if(!this->set_shadow_map_size(static_cast<u32>(this->shadow_map_size)))
        return false;
    if(!this->set_shadow_cube_size(static_cast<u32>(this->shadow_cube_size)))
        return false;
    if(!OpenGLBackend::create_uniform_buffer("camera_ubo"sv,
            sizeof(nngn::CameraUBO), &this->camera_ubo))
        return false;
    if(!OpenGLBackend::create_uniform_buffer("camera_screen_ubo"sv,
            sizeof(nngn::CameraUBO), &this->camera_screen_ubo))
        return false;
    for(size_t i = 0; i < this->shadow_camera_ubos.size(); ++i)
        if(!OpenGLBackend::create_uniform_buffer(
                "shadow_camera_ubos" + std::to_string(i),
                sizeof(nngn::CameraUBO), &this->shadow_camera_ubos[i]))
            return false;
    if(!OpenGLBackend::create_uniform_buffer("no_lights_ubo"sv,
            sizeof(nngn::LightsUBO), &this->no_lights_ubo))
        return false;
    CHECK_RESULT(glBufferSubData,
        GL_UNIFORM_BUFFER, 0, sizeof(nngn::LightsUBO),
        nngn::rptr(nngn::LightsUBO{}));
    if(!OpenGLBackend::create_uniform_buffer("lights_ubo"sv,
            sizeof(nngn::LightsUBO), &this->lights_ubo))
        return false;
    const auto light_samplers = []() {
        std::array<int, NNGN_MAX_LIGHTS> ret = {};
        std::iota(
            ret.begin(), ret.end(),
            static_cast<u32>(SHADOW_CUBE_TEX_BINDING));
        return ret;
    }();
#define P(x) static_cast<std::size_t>(PipelineConfiguration::Type::x)
    nngn::GLProgram
        &triangle_prog = this->programs[P(TRIANGLE)],
        &sprite_prog = this->programs[P(SPRITE)],
        &voxel_prog = this->programs[P(VOXEL)],
        &font_prog = this->programs[P(FONT)],
        &triangle_depth_prog = this->programs[P(TRIANGLE_DEPTH)],
        &sprite_depth_prog = this->programs[P(SPRITE_DEPTH)];
#undef P
    if(!triangle_prog.create(
            "src/glsl/gl/triangle.vert"sv, "src/glsl/gl/triangle.frag"sv,
            nngn::GLSL_GL_TRIANGLE_VERT, nngn::GLSL_GL_TRIANGLE_FRAG))
        return false;
    CHECK_RESULT(glUseProgram, triangle_prog.id());
    if(!triangle_prog.set_uniform("lights_shadow_map", SHADOW_MAP_TEX_BINDING))
        return false;
    if(!triangle_prog.set_uniform(
            "lights_shadow_cube", light_samplers.size(), light_samplers.data()))
        return false;
    if(!triangle_prog.get_uniform_location(
            "alpha", &this->triangle_prog_alpha_loc))
        return false;
    if(!triangle_prog.bind_ubo("Camera", CAMERA_UBO_BINDING))
        return false;
    if(!triangle_prog.bind_ubo("Lights", LIGHTS_UBO_BINDING))
        return false;
    if(!triangle_depth_prog.create(
            "src/glsl/gl/triangle_depth.vert"sv,
            "src/glsl/gl/triangle_depth.frag"sv,
            nngn::GLSL_GL_TRIANGLE_DEPTH_VERT,
            nngn::GLSL_GL_TRIANGLE_DEPTH_FRAG))
        return false;
    CHECK_RESULT(glUseProgram, triangle_depth_prog.id());
    if(!triangle_depth_prog.bind_ubo("Camera", CAMERA_UBO_BINDING))
        return false;
    if(!sprite_prog.create(
            "src/glsl/gl/sprite.vert"sv, "src/glsl/gl/sprite.frag"sv,
            nngn::GLSL_GL_SPRITE_VERT, nngn::GLSL_GL_SPRITE_FRAG))
        return false;
    CHECK_RESULT(glUseProgram, sprite_prog.id());
    if(!sprite_prog.set_uniform("lights_shadow_map", SHADOW_MAP_TEX_BINDING))
        return false;
    if(!sprite_prog.set_uniform("lights_shadow_cube",
            light_samplers.size(), light_samplers.data()))
        return false;
    if(!sprite_prog.bind_ubo("Camera", CAMERA_UBO_BINDING))
        return false;
    if(!sprite_prog.bind_ubo("Lights", LIGHTS_UBO_BINDING))
        return false;
    if(!voxel_prog.create(
            "src/glsl/gl/sprite.vert"sv, "src/glsl/gl/voxel.frag"sv,
            nngn::GLSL_GL_SPRITE_VERT, nngn::GLSL_GL_VOXEL_FRAG))
        return false;
    CHECK_RESULT(glUseProgram, voxel_prog.id());
    if(!voxel_prog.set_uniform("lights_shadow_map", SHADOW_MAP_TEX_BINDING))
        return false;
    if(!voxel_prog.set_uniform("lights_shadow_cube",
            light_samplers.size(), light_samplers.data()))
        return false;
    if(!voxel_prog.bind_ubo("Camera", CAMERA_UBO_BINDING))
        return false;
    if(!voxel_prog.bind_ubo("Lights", LIGHTS_UBO_BINDING))
        return false;
    if(!sprite_depth_prog.create(
            "src/glsl/gl/sprite_depth.vert"sv,
            "src/glsl/gl/sprite_depth.frag"sv,
            nngn::GLSL_GL_SPRITE_DEPTH_VERT,
            nngn::GLSL_GL_SPRITE_DEPTH_FRAG))
        return false;
    CHECK_RESULT(glUseProgram, sprite_depth_prog.id());
    if(!sprite_depth_prog.bind_ubo("Camera", CAMERA_UBO_BINDING))
        return false;
    if(!font_prog.create(
            "src/glsl/gl/font.vert"sv, "src/glsl/gl/font.frag"sv,
            nngn::GLSL_GL_FONT_VERT, nngn::GLSL_GL_FONT_FRAG))
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

bool OpenGLBackend::create_uniform_buffer(
    std::string_view name, u64 size, nngn::GLBuffer *b
) {
    return b->create(GL_UNIFORM_BUFFER, size, GL_DYNAMIC_DRAW)
        && nngn::gl_set_obj_name(NNGN_GL_BUFFER, b->id(), name);
}

bool OpenGLBackend::set_shadow_map_size(u32 s) {
    NNGN_LOG_CONTEXT_CF(OpenGLBackend);
    this->shadow_map_size = static_cast<GLsizei>(s);
    this->shadow_map.destroy();
    CHECK_RESULT(glGenTextures, 1, &this->shadow_map.id());
    CHECK_RESULT(glActiveTexture, GL_TEXTURE0 + SHADOW_MAP_TEX_BINDING);
    return this->shadow_map.init(s);
}

bool OpenGLBackend::set_shadow_cube_size(u32 s) {
    NNGN_LOG_CONTEXT_CF(OpenGLBackend);
    this->shadow_cube_size = static_cast<GLsizei>(s);
    static_assert(
        sizeof(this->shadow_cubes[0]) == sizeof(u32),
        "cannot create all textures with one command");
    constexpr auto n = NNGN_MAX_LIGHTS;
    CHECK_RESULT(glDeleteTextures, n, &this->shadow_cubes[0].id());
    CHECK_RESULT(glGenTextures, n, &this->shadow_cubes[0].id());
    auto binding = GL_TEXTURE0 + SHADOW_CUBE_TEX_BINDING;
    for(auto &x : this->shadow_cubes) {
        CHECK_RESULT(glActiveTexture, binding++);
        if(!x.init(s))
            return false;
    }
    return true;
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
    f(&this->render_list.depth, l.depth);
    f(&this->render_list.map_ortho, l.map_ortho);
    f(&this->render_list.map_persp, l.map_persp);
    f(&this->render_list.normal, l.normal);
    f(&this->render_list.overlay, l.overlay);
    f(&this->render_list.screen, l.screen);
    f(&this->render_list.shadow_maps, l.shadow_maps);
    f(&this->render_list.shadow_cubes, l.shadow_cubes);
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
        {{{"position", 3}, {"normal", 3}, {"color", 3}}},
        {{{"position", 3}, {"normal", 3}, {"tex_coord", 3}}},
        {{{"position", 3}, {"normal", 3}, {"tex_coord", 3}}},
        {{{"position", 3}, {{}, 3}, {"tex_coord", 3}}},
        {{{"position", 3}, {{}, 6}, {{}, 0}}},
        {{{"position", 3}, {{}, 3}, {"tex_coord", 3}}},
    }};
    static constexpr std::array names = {
        "triangle", "sprite", "voxel", "font", "triangle_depth", "sprite_depth",
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

bool OpenGLBackend::resize_textures(u32 s) {
    NNGN_LOG_CONTEXT_CF(OpenGLBackend);
    const auto si = static_cast<GLint>(s);
    return this->tex.destroy()
        && LOG_RESULT(glActiveTexture, GL_TEXTURE0 + MAIN_TEX_BINDING)
        && LOG_RESULT(glGenTextures, 1, &this->tex.id())
        && this->tex.create(
            GL_TEXTURE_2D_ARRAY, GL_RGBA8, GL_NEAREST, GL_NEAREST, GL_REPEAT,
            {Graphics::TEXTURE_EXTENT, Graphics::TEXTURE_EXTENT, si},
            static_cast<GLsizei>(Graphics::TEXTURE_MIP_LEVELS))
        && nngn::gl_set_obj_name(GL_TEXTURE, this->tex.id(), "tex"sv);
}

bool OpenGLBackend::load_textures(u32 i, u32 n, const std::byte *v) {
    NNGN_LOG_CONTEXT_CF(OpenGLBackend);
    const u32 base_layer = i;
    constexpr auto type = GL_TEXTURE_2D_ARRAY;
    CHECK_RESULT(glActiveTexture, GL_TEXTURE0 + MAIN_TEX_BINDING);
    CHECK_RESULT(glBindTexture, type, this->tex.id());
    for(i = 0; i < n; ++i)
        CHECK_RESULT(glTexSubImage3D,
            type, 0, 0, 0, static_cast<GLint>(base_layer + i),
            Graphics::TEXTURE_EXTENT, Graphics::TEXTURE_EXTENT,
            1, GL_RGBA, GL_UNSIGNED_BYTE,
            v + static_cast<std::size_t>(i) * Graphics::TEXTURE_SIZE);
    CHECK_RESULT(glGenerateMipmap, type);
    return true;
}

bool OpenGLBackend::resize_font(u32 s) {
    NNGN_LOG_CONTEXT_CF(OpenGLBackend);
    if(!this->font_tex.destroy())
        return false;
    const auto si = static_cast<i32>(s);
    return this->font_tex.destroy()
        && LOG_RESULT(glActiveTexture, GL_TEXTURE0 + FONT_TEX_BINDING)
        && LOG_RESULT(glGenTextures, 1, &this->font_tex.id())
        && this->font_tex.create(
            GL_TEXTURE_2D_ARRAY, GL_RGBA8, GL_NEAREST, GL_NEAREST, GL_REPEAT,
            {si, si, nngn::Font::N},
            static_cast<GLsizei>(nngn::Math::mip_levels(s)))
        && nngn::gl_set_obj_name(GL_TEXTURE, this->font_tex.id(), "font_tex"sv);
}

bool OpenGLBackend::load_font(
    unsigned char c, u32 n, const nngn::uvec2 *size, const std::byte *v
) {
    NNGN_LOG_CONTEXT_CF(OpenGLBackend);
    constexpr auto type = GL_TEXTURE_2D_ARRAY;
    CHECK_RESULT(glActiveTexture, GL_TEXTURE0 + FONT_TEX_BINDING);
    CHECK_RESULT(glBindTexture, type, this->font_tex.id());
    for(size_t i = 0; i < n; ++i, ++c, v += 4_z * size->x * size->y, ++size)
        CHECK_RESULT(
            glTexSubImage3D, type, 0, 0, 0, static_cast<GLint>(c),
            static_cast<GLsizei>(size->x), static_cast<GLsizei>(size->y),
            1, GL_RGBA, GL_UNSIGNED_BYTE, v);
    CHECK_RESULT(glGenerateMipmap, type);
    return true;
}

bool OpenGLBackend::render() {
    NNGN_LOG_CONTEXT_CF(OpenGLBackend);
    const auto update_camera = [this]() {
        if(~this->flags & Flag::CAMERA_UPDATED)
            return true;
        this->flags.clear(Flag::CAMERA_UPDATED);
        nngn::CameraUBO c = {.proj = *this->camera.proj * *this->camera.view};
        CHECK_RESULT(glBindBuffer, GL_UNIFORM_BUFFER, this->camera_ubo.id());
        CHECK_RESULT(glBufferSubData, GL_UNIFORM_BUFFER, 0, sizeof(c), &c);
        c = {*this->camera.screen_proj, nngn::mat4{1}};
        CHECK_RESULT(glBindBuffer,
            GL_UNIFORM_BUFFER, this->camera_screen_ubo.id());
        CHECK_RESULT(glBufferSubData, GL_UNIFORM_BUFFER, 0, sizeof(c), &c);
        return true;
    };
    const auto update_lighting = [this]() {
        if(~this->flags & Flag::LIGHTING_UPDATED)
            return true;
        this->flags.clear(Flag::LIGHTING_UPDATED);
        const auto &ubo = *this->lighting.ubo;
        CHECK_RESULT(glBindBuffer, GL_UNIFORM_BUFFER, this->lights_ubo.id());
        CHECK_RESULT(glBufferSubData, GL_UNIFORM_BUFFER, 0, sizeof(ubo), &ubo);
        nngn::CameraUBO c = {};
        auto proj = *this->lighting.dir_proj;
        auto *dst = this->shadow_camera_ubos.data();
        const auto update_ubo = [&c, &proj, &dst](const auto &view) {
            c.proj = proj * view;
            CHECK_RESULT(glBindBuffer, GL_UNIFORM_BUFFER, (dst++)->id());
            CHECK_RESULT(glBufferSubData,
                GL_UNIFORM_BUFFER, 0, sizeof(nngn::CameraUBO), &c);
            return true;
        };
        const auto *dir_views = this->lighting.dir_views;
        for(size_t i = 0, n = ubo.n_dir; i < n; ++i)
            update_ubo(*dir_views++);
        dst = this->shadow_camera_ubos.data() + NNGN_MAX_LIGHTS;
        proj = *this->lighting.point_proj;
        const auto *point_views = this->lighting.point_views;
        for(size_t i = 0, n = ubo.n_point; i < n; ++i)
            for(size_t f = 0; f < 6; ++f)
                update_ubo(*point_views++);
        return true;
    };
    auto render = [this, cur_tex = UINT32_MAX](
        auto *l, u32 tex_i = UINT32_MAX) mutable
    {
        using PFlag = PipelineConfiguration::Flag;
        for(auto &x : *l) {
            assert(x.pipeline < this->pipelines.size());
            const auto &pipeline = this->pipelines[x.pipeline];
            if(pipeline.conf.flags & PFlag::DEPTH_TEST)
                CHECK_RESULT(glEnable, GL_DEPTH_TEST);
            else
                CHECK_RESULT(glDisable, GL_DEPTH_TEST);
            if(pipeline.conf.flags & PFlag::DEPTH_WRITE)
                CHECK_RESULT(glDepthMask, GL_TRUE);
            else
                CHECK_RESULT(glDepthMask, GL_FALSE);
            if(pipeline.conf.flags & PFlag::CULL_BACK_FACES)
                CHECK_RESULT(glEnable, GL_CULL_FACE);
            else
                CHECK_RESULT(glDisable, GL_CULL_FACE);
            const auto type = pipeline.conf.type;
            const GLenum mode = (pipeline.conf.flags & PFlag::LINE)
                ? GL_LINES : GL_TRIANGLES;
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
                const auto next_tex = tex_i == UINT32_MAX
                    ? (type == PipelineConfiguration::Type::FONT
                        ? this->font_tex.id() : this->tex.id())
                    : tex_i;
                if(next_tex != cur_tex) {
                    CHECK_RESULT(glActiveTexture,
                        GL_TEXTURE0 + MAIN_TEX_BINDING);
                    CHECK_RESULT(glBindTexture, GL_TEXTURE_2D_ARRAY, next_tex);
                    cur_tex = next_tex;
                }
                CHECK_RESULT(glDrawElements,
                    mode,
                    static_cast<GLsizei>(
                        ebo.size / static_cast<GLsizeiptr>(sizeof(u32))),
                    GL_UNSIGNED_INT, nullptr);
            }
        }
        return true;
    };
    if(!update_camera() || !update_lighting())
        return false;
    const auto depth_pass = [this, &render] {
        NNGN_ANON_DECL(nngn::GLDebugGroup{"depth"sv});
        CHECK_RESULT(glBindFramebuffer, GL_FRAMEBUFFER, this->lights_fb.id());
        CHECK_RESULT(glViewport,
            0, 0, this->shadow_map_size, this->shadow_map_size);
        CHECK_RESULT(glBindBufferRange,
            GL_UNIFORM_BUFFER, LIGHTS_UBO_BINDING,
            this->no_lights_ubo.id(), 0, sizeof(nngn::LightsUBO));
        CHECK_RESULT(glDisable, GL_CULL_FACE);
        for(size_t i = 0, n = this->lighting.ubo->n_dir; i < n; ++i) {
            CHECK_RESULT(glBindBufferRange,
                GL_UNIFORM_BUFFER, CAMERA_UBO_BINDING,
                this->shadow_camera_ubos[i].id(), 0, sizeof(nngn::CameraUBO));
            CHECK_RESULT(glFramebufferTextureLayer,
                GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                this->shadow_map.id(), 0, static_cast<GLint>(i));
            CHECK_RESULT(glDepthMask, GL_TRUE);
            CHECK_RESULT(glClear, GL_DEPTH_BUFFER_BIT);
            if(!render(&render_list.depth))
                return false;
        }
        CHECK_RESULT(glViewport,
            0, 0, this->shadow_cube_size, this->shadow_cube_size);
        for(size_t i = 0, n = this->lighting.ubo->n_point; i < n; ++i)
            for(size_t f = 0; f < 6; ++f) {
                CHECK_RESULT(glBindBufferRange,
                    GL_UNIFORM_BUFFER, CAMERA_UBO_BINDING,
                    this->shadow_camera_ubos[NNGN_MAX_LIGHTS + i * 6 + f].id(),
                    0, sizeof(nngn::CameraUBO));
                auto face = static_cast<GLenum>(
                    GL_TEXTURE_CUBE_MAP_POSITIVE_X + f);
                if(face == GL_TEXTURE_CUBE_MAP_POSITIVE_Y)
                    face = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
                else if(face == GL_TEXTURE_CUBE_MAP_NEGATIVE_Y)
                    face = GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
                CHECK_RESULT(glFramebufferTexture2D,
                    GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                    face, this->shadow_cubes[i].id(), 0);
                CHECK_RESULT(glDepthMask, GL_TRUE);
                CHECK_RESULT(glClear, GL_DEPTH_BUFFER_BIT);
                if(!render(&render_list.depth))
                    return false;
            }
        return true;
    };
    const auto render_pass = [this, &render] {
        NNGN_ANON_DECL(nngn::GLDebugGroup{"color"sv});
        CHECK_RESULT(glBindFramebuffer, GL_FRAMEBUFFER, 0);
        CHECK_RESULT(glViewport,
            0, 0,
            static_cast<GLsizei>(this->camera.screen->x),
            static_cast<GLsizei>(this->camera.screen->y));
        CHECK_RESULT(glDepthMask, GL_TRUE);
        CHECK_RESULT(
            glBindBufferRange, GL_UNIFORM_BUFFER, LIGHTS_UBO_BINDING,
            this->lights_ubo.id(), 0, sizeof(nngn::LightsUBO));
        CHECK_RESULT(glBindBufferRange,
            GL_UNIFORM_BUFFER, CAMERA_UBO_BINDING,
            this->camera_ubo.id(), 0, sizeof(nngn::CameraUBO));
        CHECK_RESULT(glClear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        const bool persp =
            *this->camera.flags & nngn::Camera::Flag::PERSPECTIVE;
        if(!render(persp ? &render_list.map_persp : &render_list.map_ortho))
            return false;
        const auto &triangle_prog = this->programs[
            static_cast<std::size_t>(PipelineConfiguration::Type::TRIANGLE)];
        CHECK_RESULT(glUseProgram, triangle_prog.id());
        CHECK_RESULT(glUniform1f, this->triangle_prog_alpha_loc, 1);
        return render(&render_list.normal);
    };
    const auto overlay_pass = [this, &render] {
        NNGN_ANON_DECL(nngn::GLDebugGroup{"overlay"sv});
        CHECK_RESULT(glDepthMask, GL_TRUE);
        CHECK_RESULT(glClear, GL_DEPTH_BUFFER_BIT);
        CHECK_RESULT(glDepthMask, GL_FALSE);
        CHECK_RESULT(
            glBindBufferRange, GL_UNIFORM_BUFFER, LIGHTS_UBO_BINDING,
            this->no_lights_ubo.id(), 0, sizeof(nngn::LightsUBO));
        const auto &triangle_prog = this->programs[
            static_cast<std::size_t>(PipelineConfiguration::Type::TRIANGLE)];
        CHECK_RESULT(glUseProgram, triangle_prog.id());
        CHECK_RESULT(glUniform1f, this->triangle_prog_alpha_loc, .5);
        if(!render(&render_list.overlay))
            return false;
        CHECK_RESULT(glBindBufferRange,
            GL_UNIFORM_BUFFER, CAMERA_UBO_BINDING,
            this->camera_screen_ubo.id(), 0, sizeof(nngn::CameraUBO));
        if(!render(&render_list.screen))
            return false;
        return render(&render_list.shadow_maps, this->shadow_map.id());
    };
    auto prof = nngn::Profile::context<nngn::Profile>(
        &nngn::Profile::stats.render);
    CHECK_RESULT(glActiveTexture, GL_TEXTURE0 + MAIN_TEX_BINDING);
    CHECK_RESULT(glBindTexture, GL_TEXTURE_2D_ARRAY, this->tex.id());
    if(this->lighting.ubo->flags & nngn::LightsUBO::SHADOWS_ENABLED)
        if(!depth_pass())
            return false;
    if(!(render_pass() && overlay_pass()))
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
