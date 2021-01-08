#ifndef NNGN_GRAPHICS_TERMINAL_CONST_H
#define NNGN_GRAPHICS_TERMINAL_CONST_H

#include "utils/literals.h"

namespace nngn::term {

using namespace nngn::literals;

/** Character sequences to control a VT100 terminal. */
struct VT100EscapeCode {
    static constexpr auto clear = "\x1b[2J"_s;
    static constexpr auto pos = "\x1b[H"_s;
};

/** Character sequences to control a VT520 terminal. */
struct VT520EscapeCode {
    static constexpr auto show_cursor = "\x1b[?25h"_s;
    static constexpr auto hide_cursor = "\x1b[?25l"_s;
};

/** ANSI escape code sequences. */
struct ANSIEscapeCode {
    static constexpr auto reset_color = "\x1b[39;49m"_s;
    static constexpr auto bg_color_24bit = "\x1b[48;2;"_s;
};

}

#endif
