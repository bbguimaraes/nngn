#include <algorithm>
#include <cassert>

#include "timing/timing.h"

#include "textbox.h"

namespace {

auto font(const nngn::Fonts *f) { assert(f); return f->fonts() + f->n() - 1; }

}

namespace nngn {

std::size_t Textbox::text_length(void) const {
    constexpr auto f = [](const auto &t) {
        const auto b = t.str.begin();
        return static_cast<u32>(std::count_if(
            b, b + static_cast<std::ptrdiff_t>(t.cur),
            static_cast<bool(*)(char)>(&Textbox::is_character)));
    };
    return f(this->title) + f(this->str);
}

void Textbox::set_title(const char *s) {
    this->title = Text(*font(this->fonts), s);
    this->flags.set(Flag::UPDATED);
}

void Textbox::set_text(const char *s) {
    this->str = Text(*font(this->fonts), s, 0);
    this->timer = std::chrono::microseconds(0);
    this->flags.set(Flag::UPDATED);
}

void Textbox::set_cur(std::size_t cur) {
    this->str.update_cur(*font(this->fonts), cur);
}

bool Textbox::update(const nngn::Timing &t) {
    this->timer += std::chrono::duration_cast<std::chrono::microseconds>(t.dt);
    const auto size = this->str.str.size();
    auto cur = this->str.cur;
    if(cur == size)
        return this->flags.is_set(Flag::SCREEN_UPDATED);
    const auto n = this->speed.count()
        ? static_cast<std::size_t>(this->timer / this->speed)
        : size;
    std::size_t c = 0;
    for(; cur != size && c != n; ++cur)
        if(Textbox::is_character(this->str.str[cur]))
            c++;
    this->timer -= this->speed * c;
    this->str.update_cur(*font(this->fonts), cur);
    this->flags.set(Flag::UPDATED);
    return true;
}

void Textbox::update_size(void) {
    const auto *const f = font(this->fonts);
    const auto pad = static_cast<float>(f->size) / 2;
    const auto title_width = this->monospaced()
        ? static_cast<float>(f->size * this->title.str.size())
        : this->title.size.x;
    const auto title_sz = 2 * pad + vec2{title_width, this->title.size.y};
    this->title_bl = this->str_bl + pad + vec2{0, this->str.size.y};
    this->title_tr = this->title_bl + title_sz;
}

void Textbox::update_size(const uvec2 &screen) {
    const auto *const f = font(this->fonts);
    constexpr std::size_t WIDTH = 800;
    const auto pad = static_cast<float>(f->size) / 2;
    const vec2 size = {WIDTH - 2 * pad, this->str.size.y + 2 * pad};
    const vec2 center = {static_cast<float>(screen.x) / 2, pad + size.y / 2};
    this->str_bl = center - size / 2.0f;
    this->str_tr = str_bl + size;
    this->update_size();
}

}
