#ifndef NNGN_GRAPHICS_PSEUDO_H
#define NNGN_GRAPHICS_PSEUDO_H

#include "graphics.h"

namespace nngn {

struct Pseudograph : Graphics {
    bool init() override { return true; }
    bool window_closed() const override { return false; }
    int swap_interval() const override { return 1; }
    void set_swap_interval(int) override {}
    void set_window_title(const char*) override {}
    void set_cursor_mode(CursorMode) override {}
    void set_key_callback(void*, key_callback_f) override {}
    void set_mouse_button_callback(void*, mouse_button_callback_f) override {}
    void set_mouse_move_callback(void*, mouse_move_callback_f) override {}
    void resize(int, int) override {}
    void poll_events() const override {}
    bool render() override;
};

}

#endif
