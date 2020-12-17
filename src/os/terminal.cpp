#include "terminal.h"

#include "os/platform.h"

#ifdef HAVE_TERMIOS_H

#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "os.h"

namespace nngn {

Terminal::~Terminal(void) {
    if(this->fd == -1)
        return;
    this->write(VT520EscapeCode::show_cursor);
    this->write(1, "\n");
    if(close(this->fd) == -1)
        nngn::Log::perror("close");
    if(this->tty_fd != this->fd && close(this->tty_fd) == -1)
        nngn::Log::perror("close(/dev/tty)");
}

bool Terminal::init(int src_fd) {
    NNGN_LOG_CONTEXT_CF(Terminal);
    if((this->fd = dup(src_fd)) == -1)
        return nngn::Log::perror("dup"), false;
    if(!(this->f = fdopen(this->fd, "w")))
        return nngn::Log::perror("fdopen"), false;
    const int is_tty = isatty(this->fd);
    if(!is_tty && errno != ENOTTY)
        return nngn::Log::perror("isatty"), false;
    if(is_tty)
        this->tty_fd = this->fd;
    else if((this->tty_fd = open("/dev/tty", O_RDONLY | O_CLOEXEC)) == -1)
        return nngn::Log::perror("open(/dev/tty)"), false;
    return true;
}

std::tuple<bool, bool> Terminal::update_size(void) {
    NNGN_LOG_CONTEXT_CF(Terminal);
    winsize ws = {};
    std::tuple<bool, bool> ret = {};
    if(ioctl(this->tty_fd, TIOCGWINSZ, &ws) == -1)
        return nngn::Log::perror("ioctl(TIOCGWINSZ)"), ret;
    const auto has_pixel_size = ws.ws_xpixel && ws.ws_ypixel;
    if(!has_pixel_size)
        ws.ws_xpixel = ws.ws_col * 8, ws.ws_ypixel = ws.ws_row * 16;
    const auto old =
        std::exchange(this->m_size, {ws.ws_col, ws.ws_row});
    const auto old_px =
        std::exchange(this->m_pixel_size, {ws.ws_xpixel, ws.ws_ypixel});
    const auto changed = this->m_size != old || this->m_pixel_size != old_px;
    std::get<0>(ret) = changed;
    std::get<1>(ret) = true;
    if(changed && !has_pixel_size) {
        nngn::Log::l() << "TIOCGWINSZ: ws_xpixel and/or ws_ypixel are zero\n";
        nngn::Log::l() << "using characters which are 8x16 pixels instead\n";
    }
    return ret;
}

bool Terminal::write(std::size_t n, const char *p) const {
    constexpr std::size_t max = /*XXX!?*/1;
    for(std::size_t w = 0; n; n -= w, p += static_cast<std::size_t>(w)) {
        const auto nw = std::min(n, max);
        w = fwrite(p, 1, nw, this->f);
        if(w != nw && ferror(this->f) && errno != EAGAIN)
            return nngn::Log::perror("write"), false;
    }
    return true;
}

bool Terminal::flush(void) const {
    NNGN_LOG_CONTEXT_CF(Terminal);
    while(fflush(this->f))
        if(errno != EAGAIN)
            return nngn::Log::perror("fsync"), false;
    return true;
}

bool Terminal::drain(void) const {
    NNGN_LOG_CONTEXT_CF(Terminal);
    while(tcdrain(this->tty_fd))
        if(errno != EAGAIN)
            return nngn::Log::perror("tcdrain"), false;
    return true;
}

}
#endif // HAVE_TERMIOS_H
