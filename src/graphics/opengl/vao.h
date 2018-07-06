#ifndef NNGN_GRAPHICS_OPENGL_VAO_H
#define NNGN_GRAPHICS_OPENGL_VAO_H

#include "handle.h"
#include "opengl.h"

namespace nngn {

struct GLProgram;

struct VAO final : nngn::OpenGLHandle<VAO> {
    struct Attrib { std::string_view name; GLsizei size; };
    u32 vbo = {}, ebo = {};
    bool create(u32 vbo, u32 ebo);
    bool destroy();
    bool vertex_attrib_pointers(
        const GLProgram &prog, std::size_t n, const Attrib *p);
};

}

#endif
