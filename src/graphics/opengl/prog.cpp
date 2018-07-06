#include "os/platform.h"

#ifdef NNGN_PLATFORM_HAS_OPENGL
#include "prog.h"

#include <vector>

#include "utils/log.h"

#include "utils.h"

using namespace std::string_literals;

namespace nngn {

bool GLShader::create(
    GLenum type, std::string_view name, std::span<const u8> src
) {
    NNGN_LOG_CONTEXT_CF(GLShader);
    NNGN_LOG_CONTEXT(name.data());
    if(!(this->id() = glCreateShader(type)))
        return gl_check_result("glCreateShader"), false;
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
        gl_check_result("glCreateProgram");
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

bool GLProgram::create(
    std::string_view vs_name, std::string_view fs_name,
    std::span<const u8> vs, std::span<const u8> fs
) {
    NNGN_LOG_CONTEXT_CF(GLProgram);
    NNGN_LOG_CONTEXT(vs_name.data());
    NNGN_LOG_CONTEXT(fs_name.data());
    GLShader v, f;
    return v.create(GL_VERTEX_SHADER, vs_name, vs)
        && gl_set_obj_name(NNGN_GL_SHADER, v.id(), vs_name)
        && f.create(GL_FRAGMENT_SHADER, fs_name, fs)
        && gl_set_obj_name(NNGN_GL_SHADER, f.id(), fs_name)
        && this->create(v.id(), f.id())
        && gl_set_obj_name(
            NNGN_GL_PROGRAM, this->id(),
            vs_name.data() + "+"s + fs_name.data());
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
    gl_check_result("glGetUniformLocation");
    return false;
}

bool GLProgram::set_uniform(const char *name, int v) const {
    NNGN_LOG_CONTEXT_CF(GLProgram);
    int l = {};
    if(!this->get_uniform_location(name, &l))
        return false;
    CHECK_RESULT(glUniform1i, l, v);
    return true;
}

bool GLProgram::set_uniform(const char *name, GLsizei n, const int *v) const {
    NNGN_LOG_CONTEXT_CF(GLProgram);
    int l = {};
    if(!this->get_uniform_location(name, &l))
        return false;
    CHECK_RESULT(glUniform1iv, l, n, v);
    return true;
}

bool GLProgram::bind_ubo(const char *name, u32 binding) const {
    if(const auto i = glGetUniformBlockIndex(this->id(), name);
            i != GL_INVALID_INDEX) {
        CHECK_RESULT(glUniformBlockBinding, this->id(), i, binding);
        return true;
    }
    gl_check_result("glGetUniformBlockIndex");
    nngn::Log::l() << "glGetUniformBlockIndex: GL_INVALID_INDEX" << std::endl;
    return false;
}

}
#endif
