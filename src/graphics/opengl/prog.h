#ifndef NNGN_GRAPHICS_OPENGL_PROGRAM_H
#define NNGN_GRAPHICS_OPENGL_PROGRAM_H

#include <span>
#include <string_view>

#include "utils/def.h"

#include "handle.h"
#include "opengl.h"

namespace nngn {

struct GLShader final : nngn::OpenGLHandle<GLShader> {
    bool create(GLenum type, std::string_view name, std::span<const u8> src);
    bool destroy();
};

struct GLProgram : OpenGLHandle<GLProgram> {
    bool create(u32 vert, u32 frag);
    bool create(
        std::string_view vs_name, std::string_view fs_name,
        std::span<const u8> vs, std::span<const u8> fs);
    bool destroy();
    bool get_uniform_location(const char *name, int *l) const;
    bool set_uniform(const char *name, int v) const;
    bool set_uniform(const char *name, GLsizei n, const int *v) const;
    bool bind_ubo(const char *name, u32 binding) const;
};

}

#endif
