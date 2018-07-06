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

}
#endif
