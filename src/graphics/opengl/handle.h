#ifndef NNGN_GRAPHICS_OPENGL_HANDLE_H
#define NNGN_GRAPHICS_OPENGL_HANDLE_H

#include "utils/def.h"

#include "opengl.h"

namespace nngn {

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

}

#endif
