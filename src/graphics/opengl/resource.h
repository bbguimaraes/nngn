#ifndef NNGN_GRAPHICS_OPENGL_RESOURCE_H
#define NNGN_GRAPHICS_OPENGL_RESOURCE_H

#include <span>

#include "graphics/graphics.h"
#include "math/vec3.h"

#include "handle.h"

namespace nngn {

struct GLBuffer final : OpenGLHandle<GLBuffer> {
    using Configuration = Graphics::BufferConfiguration;
    GLenum target = {}, usage = {};
    GLsizeiptr size = 0, capacity = 0;
    bool create(GLenum target, u64 size, GLenum usage);
    bool create(GLenum target, std::span<const std::byte> data, GLenum usage);
    bool create(const Configuration &conf);
    bool set_capacity(u64 n);
    bool destroy();
};

struct GLTexArray : OpenGLHandle<GLTexArray> {
    bool create(
        GLenum type, GLenum fmt, GLint min_filter, GLint mag_filter, GLint wrap,
        const ivec3 &extent, GLsizei mip_levels);
    bool destroy();
};

}

#endif
