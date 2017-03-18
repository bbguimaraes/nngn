#ifndef NNGN_GRAPHICS_PSEUDO_H
#define NNGN_GRAPHICS_PSEUDO_H

#include "graphics.h"

namespace nngn {

struct Pseudograph : Graphics {
    bool init() override { return true; }
    bool window_closed() const override { return false; }
    int swap_interval() const override { return 1; }
    void set_swap_interval(int) override {}
    void resize(int, int) override {}
    void poll_events() const override {}
    bool render() override;
};

}

#endif
