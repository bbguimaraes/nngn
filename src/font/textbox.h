#ifndef NNGN_TEXTBOX_H
#define NNGN_TEXTBOX_H

#include <chrono>

#include "math/vec2.h"
#include "utils/def.h"
#include "utils/flags.h"

#include "text.h"

namespace nngn {

class Fonts;
struct Timing;

class Textbox {
public:
    enum Flag : u8 {
        UPDATED = 1u << 0,
        SCREEN_UPDATED = 1u << 1,
        MONOSPACED = 1u << 2,
    };
    static constexpr auto DEFAULT_SPEED = std::chrono::milliseconds(50);
    Text str = {}, title = {};
    std::chrono::microseconds timer = {};
    std::chrono::milliseconds speed = DEFAULT_SPEED;
    vec2 title_bl = {0, 0}, title_tr = {0, 0};
    vec2 str_bl = {0, 0}, str_tr = {0, 0};
    static bool is_character(unsigned char c);
    static bool is_character(char c);
    void init(const Fonts *f) { this->fonts = f; }
    bool empty(void) const;
    std::size_t text_length(void) const;
    bool monospaced(void) const { return this->flags.is_set(Flag::MONOSPACED); }
    bool updated(void) const;
    bool finished(void) const { return this->str.cur == this->str.str.size(); }
    void set_monospaced(bool m);
    void set_screen_updated(void) { this->flags.set(Flag::SCREEN_UPDATED); }
    void set_speed(unsigned s) { this->speed = std::chrono::milliseconds(s); }
    void set_title(const char *s);
    void set_text(const char *s);
    void set_cur(std::size_t cur);
    bool update(const Timing &t);
    void update_size(void);
    void update_size(const uvec2 &screen);
    void clear_updated(void);
private:
    Flags<Flag> flags = {};
    const Fonts *fonts = nullptr;
};

inline bool Textbox::is_character(unsigned char c) {
    return c != '\n';
}

inline bool Textbox::is_character(char c) {
    return Textbox::is_character(static_cast<unsigned char>(c));
}

inline bool Textbox::empty(void) const {
    return this->str.str.empty() && this->title.str.empty();
}

inline void Textbox::set_monospaced(bool m) {
    this->flags.set(Flag::MONOSPACED, m);
    this->flags.set(Flag::UPDATED);
    this->update_size();
}

inline bool Textbox::updated(void) const {
    return this->flags.is_set(Flag::UPDATED | Flag::SCREEN_UPDATED);
}

inline void Textbox::clear_updated(void) {
    this->flags.clear(Flag::UPDATED | Flag::SCREEN_UPDATED);
}

}

#endif
