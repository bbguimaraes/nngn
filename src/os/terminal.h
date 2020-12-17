#ifndef NNGN_OS_TERMINAL_H
#define NNGN_OS_TERMINAL_H

#include <ranges>

#include "math/vec2.h"
#include "utils/log.h"
#include "utils/utils.h"

#include "os.h"

namespace nngn {

using nngn::uvec2;

/** Handles interactions with the output terminal. */
class Terminal {
public:
    NNGN_MOVE_ONLY(Terminal)
    Terminal(void) = default;
    ~Terminal(void);
    /** Size of the terminal in characters. */
    auto size(void) const { return this->m_size; }
    /** Size of the terminal in pixels. */
    auto pixel_size(void) const { return this->m_pixel_size; }
    /**
     * Creates an object for a given TTY.
     * \param fd Output file descriptor, `dup(2)`ed internally.
     */
    bool init(int fd);
    /**
      * Retrieves the terminal size from the operating system.
      * \return {changed, ok}
      */
    std::tuple<bool, bool> update_size(void);
    /** Outputs the entire contents of a buffer. */
    bool write(std::size_t n, const char *p) const;
    /** Outputs the entire contents of a range. */
    bool write(const std::ranges::sized_range auto &v) const;
    bool flush(void) const;
    bool drain(void) const;
    bool show_cursor(void) const;
    bool hide_cursor(void) const;
private:
    /** OS file descriptor. */
    int fd = -1;
    /** `fdopen(3)`ed version of \ref fd. */
    // XXX why do terminals handle writes via `fwrite` much better?
    FILE *f = nullptr;
    /**
      * File descriptor for the controlling TTY.
      * Different from `fd` if `!isatty(fd)`.
      */
    int tty_fd = -1;
    /** Size of the terminal in characters. */
    uvec2 m_size = {};
    /** Size of the terminal in screen pixels, if available. */
    uvec2 m_pixel_size = {};
};

bool Terminal::write(const std::ranges::sized_range auto &v) const {
    return this->write(std::size(v), std::data(v));
}

inline bool Terminal::show_cursor(void) const {
    return this->write(nngn::VT520EscapeCode::show_cursor);
}

inline bool Terminal::hide_cursor(void) const {
    return this->write(nngn::VT520EscapeCode::hide_cursor);
}

}

#endif
