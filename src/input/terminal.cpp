#include "input.h"

#include "os/platform.h"
#include "utils/log.h"

#ifndef HAVE_TERMIOS_H

namespace nngn {

std::unique_ptr<Input::Source> input_terminal_source(int, Input::TerminalFlag) {
    NNGN_LOG_CONTEXT_F();
    Log::l() << "compiled without termios support\n";
    return {};
}

}

#else

#include <cstring>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

namespace {

std::tuple<std::int32_t, nngn::Input::Modifier> map_key(char c) {
    const auto i = static_cast<std::int32_t>(static_cast<unsigned char>(c));
    switch(c) {
    case 27: return {nngn::Input::KEY_ESC, {}};
    }
    if(1 <= c && c <= 26)
        return {i - 1 + 'A', nngn::Input::Modifier::MOD_CTRL};
    if('A' <= c && c <= 'Z')
        return {i, nngn::Input::Modifier::MOD_SHIFT};
    if('a' <= c && c <= 'z')
        return {i - 'a' + 'A', {}};
    return {c, {}};
}

struct Terminal {
    using Flag = nngn::Input::TerminalFlag;
    NNGN_NO_COPY(Terminal)
    Terminal(Terminal &&rhs) noexcept { *this = std::move(rhs); }
    Terminal(int fd_, Flag flags_) : fd{fd_}, flags{flags_} {}
    ~Terminal(void);
    Terminal &operator=(Terminal &&rhs) noexcept;
    bool init(void);
    bool read(char *c) const;
private:
    int fd = -1;
    Flag flags = {};
    termios t0 = {}, t = {};
};

class TerminalSource : public nngn::Input::Source {
public:
    explicit TerminalSource(Terminal &&t) : terminal{std::move(t)} {}
    TerminalSource(int fd, Terminal::Flag flags) : terminal(fd, flags) {}
    bool init() { return this->terminal.init(); }
    bool update(nngn::Input *input) override;
private:
    Terminal terminal;
};

Terminal::~Terminal() {
    NNGN_LOG_CONTEXT_CF(Terminal);
    if(tcsetattr(this->fd, TCSAFLUSH, &this->t0) == -1)
        nngn::Log::perror("tcsetattr");
    if(fcntl(this->fd, F_SETFL, O_NONBLOCK) == -1)
        nngn::Log::perror("fcntl");
}

Terminal &Terminal::operator=(Terminal &&rhs) noexcept {
    this->fd = std::exchange(rhs.fd, -1);
    this->flags = rhs.flags;
    return *this;
}

bool Terminal::init() {
    NNGN_LOG_CONTEXT_CF(Terminal);
    if(fcntl(this->fd, F_SETFL, O_NONBLOCK) == -1)
        return nngn::Log::perror("fcntl"), false;
    if(tcgetattr(this->fd, &this->t0) == -1)
        return nngn::Log::perror("tcgetattr"), false;
    this->t = this->t0;
    this->t.c_iflag &=
        static_cast<tcflag_t>(~(BRKINT | ICRNL | INPCK | ISTRIP | IXON));
    if(~this->flags & Flag::OUTPUT_PROCESSING)
        this->t.c_oflag &= static_cast<tcflag_t>(~OPOST);
    this->t.c_cflag |= CS8;
    this->t.c_lflag &= static_cast<tcflag_t>(~(ECHO | ICANON | IEXTEN | ISIG));
    if(tcsetattr(this->fd, TCSAFLUSH, &this->t) == -1)
        return nngn::Log::perror("tcsetattr"), false;
    return true;
}

bool Terminal::read(char *c) const {
    return ::read(this->fd, c, 1) != -1
        || errno == EWOULDBLOCK
        || (nngn::Log::perror("read"), false);
}

bool TerminalSource::update(nngn::Input *input) {
    for(;;) {
        char c = {};
        if(!this->terminal.read(&c))
            return false;
        if(!c)
            break;
        const auto [key, mod] = map_key(c);
        input->key_callback(key, nngn::Input::Action::KEY_PRESS, mod);
        input->key_callback(key, nngn::Input::Action::KEY_RELEASE, mod);
    }
    return true;
}

}

namespace nngn {

std::unique_ptr<Input::Source> input_terminal_source(
    int fd, Input::TerminalFlag flags
) {
    auto ret = std::make_unique<TerminalSource>(fd, flags);
    if(!ret->init())
        return {};
    return ret;
}

}

#endif
