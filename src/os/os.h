/**
 * \dir src/os
 * \brief Platform configuration, operating system utilities.
 *
 * This module performs two main functions:
 *
 * - Platform-specific configuration directives.  This includes the `autotools`
 *   `config.h` header and other preprocessor detection directives, presented in
 *   a centralized C/++ interface.
 * - Access to operating system resources, where available.
 */
#ifndef NNGN_OS_H
#define NNGN_OS_H

#include "utils/literals.h"

namespace nngn {

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
