#include "os/platform.h"

#ifdef NNGN_PLATFORM_HAS_OPENGL
#include "vao.h"

#include "utils/log.h"

#include "prog.h"
#include "utils.h"

namespace nngn {

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
                gl_check_result("glGetAttribLocation");
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

}
#endif
