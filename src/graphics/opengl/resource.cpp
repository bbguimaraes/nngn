#include "os/platform.h"

#ifdef NNGN_PLATFORM_HAS_OPENGL
#include "resource.h"

#include "utils/log.h"

#include "utils.h"

namespace nngn {

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

bool GLBuffer::create(
    GLenum target_, std::span<const std::byte> data, GLenum usage_
) {
    if(!this->create(target_, data.size(), usage_))
        return false;
    return LOG_RESULT(glBufferData,
        target_, static_cast<GLsizei>(data.size()), data.data(), usage_);
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

bool GLBuffer::create(const Configuration &conf) {
    return this->create(
        conf.type == Configuration::Type::VERTEX
            ? GL_ARRAY_BUFFER : GL_ELEMENT_ARRAY_BUFFER,
        conf.size, GL_DYNAMIC_DRAW);
}

bool GLBuffer::destroy() {
    NNGN_LOG_CONTEXT_CF(GLBuffer);
    if(this->id())
        CHECK_RESULT(glDeleteBuffers, 1, &this->id());
    return true;
}

bool GLTexArray::create(
    GLenum type, GLenum fmt, GLint min_filter, GLint mag_filter, GLint wrap,
    const ivec3 &extent, GLsizei mip_levels
) {
    CHECK_RESULT(glBindTexture, type, this->id());
    switch(type) {
    case GL_TEXTURE_2D:
        CHECK_RESULT(glTexStorage2D, GL_TEXTURE_2D, 1, fmt, extent.x, extent.y);
        break;
    case GL_TEXTURE_CUBE_MAP:
        CHECK_RESULT(glTexStorage2D,
            GL_TEXTURE_CUBE_MAP, 1, fmt, extent.x, extent.y);
        break;
    default:
        CHECK_RESULT(glTexStorage3D,
            type, mip_levels, fmt, extent.x, extent.y, extent.z);
    }
    CHECK_RESULT(glTexParameteri, type, GL_TEXTURE_MIN_FILTER, min_filter);
    CHECK_RESULT(glTexParameteri, type, GL_TEXTURE_MAG_FILTER, mag_filter);
    CHECK_RESULT(glTexParameteri, type, GL_TEXTURE_WRAP_R, wrap);
    CHECK_RESULT(glTexParameteri, type, GL_TEXTURE_WRAP_S, wrap);
    CHECK_RESULT(glTexParameteri, type, GL_TEXTURE_WRAP_T, wrap);
#ifdef GL_VERSION_2
    if(wrap == GL_CLAMP_TO_BORDER) {
        constexpr float border[] = {1, 1, 1, 1};
        CHECK_RESULT(glTexParameterfv, type, GL_TEXTURE_BORDER_COLOR, border);
    }
#endif
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

bool GLFrameBuffer::destroy() {
    NNGN_LOG_CONTEXT_CF(GLFrameBuffer);
    CHECK_RESULT(glDeleteFramebuffers, 1, &this->id());
    this->id() = 0;
    return true;
}

}
#endif
