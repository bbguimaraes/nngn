#include <algorithm>
#include <cassert>
#include <numeric>

#include "font.h"
#include "text.h"

namespace nngn {

void Text::update_cur(const Font &f, std::size_t c) {
    assert(c <= this->str.size());
    this->cur = c;
    this->nlines = Text::count_lines(this->str, this->cur);
    this->update_size(f);
}

void Text::update_size(const Font &f) {
    const auto flines = static_cast<float>(this->nlines);
    const auto fsize = static_cast<float>(f.size);
    this->size = {
        Text::max_width(f.chars, this->str, this->cur),
        flines * (fsize + this->spacing) - this->spacing,
    };
}

std::size_t Text::count_lines(std::string_view s, std::size_t cur) {
    assert(cur <= s.size());
    const auto *const b = s.cbegin();
    return 1 + static_cast<std::size_t>(
        std::count(b, b + static_cast<std::ptrdiff_t>(cur), '\n'));
}

float Text::max_width(
        const Font::Characters &font,
        std::string_view str, std::size_t cur) {
    assert(!cur || font.size() >= 128);
    const auto *const end = str.cbegin() + static_cast<std::ptrdiff_t>(cur);
    assert(end <= str.cend());
    const auto acc = [&font](float a, char c) {
        return static_cast<signed char>(c) < 0
            ? 0 : a + font[static_cast<std::size_t>(c)].advance;
    };
    float ret = 0.0f;
    for(const auto *it = str.cbegin(); it <= end;) {
        const auto *const e = std::find(it, end, '\n');
        ret = std::max(ret, std::accumulate(it, e, 0.0f, acc));
        it = e + 1;
    }
    return ret;
}

}
