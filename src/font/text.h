#ifndef NNGN_TEXT_H
#define NNGN_TEXT_H

#include <string>

#include "math/vec2.h"

#include "font.h"

namespace nngn {

struct Text {
    std::string str = {};
    size_t cur = 0, nlines = 0;
    float spacing = 0;
    vec2 size = {0, 0};
    constexpr Text() = default;
    Text(const Font &f, std::string_view s);
    Text(const Font &f, std::string_view s, size_t cur);
    void update_cur(const Font &f, size_t cur);
    void update_size(const Font &f);
    static size_t count_lines(std::string_view s, size_t cur);
    static float max_width(
        const Font::Characters &font, std::string_view s, size_t cur);
};

inline Text::Text(const Font &f, std::string_view s)
    : Text(f, s, s.size()) {}
inline Text::Text(const Font &f, std::string_view s, size_t c)
    : str(s), spacing(static_cast<float>(f.size) / 4.0f)
    { this->update_cur(f, c); }

}

#endif
