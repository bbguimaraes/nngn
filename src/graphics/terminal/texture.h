#ifndef NNGN_GRAPHICS_TERMINAL_TEXTURE_H
#define NNGN_GRAPHICS_TERMINAL_TEXTURE_H

#include <cfloat>

#include "math/math.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include "math/vec4.h"
#include "utils/def.h"
#include "utils/literals.h"

namespace nngn::term {

using namespace nngn::literals;

/** Buffer holding texture image data. */
class Texture {
public:
    using texel3 = vec3_base<u8>;
    using texel4 = vec4_base<u8>;
    explicit Texture(uvec2 size = {});
    uvec2 size(void) const { return this->m_size; }
    std::span<const texel4> data(void) const { return this->m_data; }
    void resize(uvec2 size);
    void copy(const u8 *p);
    void copy(uvec2 size, const u8 *p);
    texel4 sample(vec2 uv) const;
private:
    uvec2 m_size;
    vec2 size_f;
    std::vector<texel4> m_data;
};

inline Texture::Texture(uvec2 s) :
    m_size{s},
    size_f{static_cast<vec2>(s) - FLT_EPSILON * static_cast<vec2>(s)},
    m_data(4_z * Math::product(s))
{}

inline void Texture::resize(uvec2 s) { *this = Texture{s}; }

inline void Texture::copy(const u8 *p) {
    std::copy(p, p + this->m_data.size(), this->m_data.front().data());
}

inline void Texture::copy(uvec2 size, const u8 *p) {
    auto *dst = this->m_data.front().data();
    for(auto y = 0u; y != size.y; ++y) {
        std::copy(p, p + 4_z * size.x, dst);
        p += 4_z * size.x;
        dst += 4_z * this->m_size.x;
    }
}

inline auto Texture::sample(vec2 uv) const -> texel4 {
    uv.x = std::clamp(uv.x, 0.0f, 1.0f);
    uv.y = std::clamp(uv.y, 0.0f, 1.0f);
    const auto p = static_cast<zvec2>(uv * this->size_f);
    return this->m_data[this->m_size.x * p.y + p.x];
}

}

#endif
