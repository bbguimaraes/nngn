#ifndef NNGN_GRAPHICS_TERMINAL_TERMINAL_H
#define NNGN_GRAPHICS_TERMINAL_TERMINAL_H

#include "math/vec2.h"
#include "utils/log.h"
#include "utils/utils.h"

namespace nngn::term {

using nngn::uvec2;

/** Handles interactions with the output terminal. */
class Terminal {
public:
    NNGN_MOVE_ONLY(Terminal)
    /**
     * Creates an object for a given TTY.
     * \param fd
     *     OS file descriptor, whose lifetime is not managed and must remain
     *     valid until the object is destructed.
     */
    Terminal(int fd);
    ~Terminal(void);
    /** Size of the terminal in characters. */
    auto size(void) const { return this->m_size; }
    /** Size of the terminal in pixels. */
    auto pixel_size(void) const { return this->pixel; }
    bool init(void);
    /** Asks the OS for the terminal size.  Returns {changed, ok}. */
    std::tuple<bool, bool> update_size(void);
    /** Outputs the entire contents of a buffer. */
    bool write(std::size_t n, const char *p) const;
    /** Outputs the entire contents of a container. */
    template<typename T>
    bool write(const T &v) const { return this->write(v.size(), v.data()); }
    /** Synchronizes the output file descriptor. */
    bool flush(void) const;
    bool drain(void) const;
private:
    /** OS file descriptor. */
    int fd = -1;
    int tty_fd = -1;
    /** `fopen`ed version of \ref fd. */
    std::FILE *f = nullptr;
    uvec2 m_size = {};
    uvec2 pixel = {};
};

inline Terminal::Terminal(int fd_) : fd(fd_) {}

inline bool Terminal::write(std::size_t n, const char *p) const {
    constexpr std::size_t max = 1;
    for(std::size_t w = 0; n; n -= w, p += static_cast<std::ptrdiff_t>(w)) {
        w = fwrite(p, 1, std::min(n, max), this->f);
//        if(w != n) {
//            nngn::Log::l() << w << ' ' << n << '\n';
//            return false;
//        }
//        if(ferror(this->f) && errno != EAGAIN)
//            return nngn::Log::perror("fwrite"), false;
    }
    return true;
}

}

#endif
