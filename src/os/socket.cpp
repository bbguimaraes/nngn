#include "socket.h"

#include "os/platform.h"

#ifndef NNGN_PLATFORM_HAS_SOCKETS

namespace nngn {

Socket::lock::~lock() {}
Socket::Socket() {}
Socket::~Socket() {}
bool Socket::init(std::string_view) { return true; }
bool Socket::process(std::string*) { return true; }

}

#else

#include <algorithm>

#include <cassert>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "socket.h"

#include "utils/log.h"

namespace {

constexpr int BACKLOG = 8;

auto to_poll_data(std::vector<std::byte> *v) {
    assert(v->size() == BACKLOG * sizeof(pollfd));
    return static_cast<std::array<pollfd, BACKLOG>*>(
        static_cast<void*>(v->data()));
}

}

namespace nngn {

Socket::lock::~lock() {
    if(this->fd == -1)
        return;
    NNGN_LOG_CONTEXT_CF(Socket);
    NNGN_LOG_CONTEXT(this->m_path.c_str());
    if(close(this->fd) == -1)
        Log::perror("close");
    else if(!this->m_path.empty() && unlink(this->m_path.c_str()) == -1)
        Log::perror("unlink");
}

bool Socket::lock::path(std::string_view p) {
    constexpr auto flags = O_RDONLY | O_CREAT | O_CLOEXEC;
    constexpr auto uw_mode = 0600;
    if((this->fd = open(p.data(), flags, uw_mode)) == -1)
        return Log::perror("open"), false;
    if(flock(this->fd, LOCK_EX | LOCK_NB) == -1) {
        if(errno != EWOULDBLOCK)
            Log::perror("flock");
        return false;
    }
    this->m_path = p;
    return true;
}

Socket::Socket() {
    this->poll_data.resize(BACKLOG * sizeof(pollfd));
    for(auto &x : *to_poll_data(&this->poll_data))
        x = {-1, POLLIN, 0};
}

Socket::~Socket() {
    if(this->fd == -1)
        return;
    NNGN_LOG_CONTEXT_CF(Socket);
    NNGN_LOG_CONTEXT(this->path.c_str());
    if(close(this->fd) == -1)
        Log::perror("close(fd)");
    else if(unlink(this->path.c_str()) == -1)
        Log::perror("unlink");
    for(const auto &x : *to_poll_data(&this->poll_data))
        if(x.fd != -1 && close(x.fd) == -1)
            Log::perror("close(pollfd)");
}

bool Socket::init(std::string_view p) {
    constexpr auto MAX_LEN = 104u - 1;
    NNGN_LOG_CONTEXT_CF(Socket);
    NNGN_LOG_CONTEXT(p.data());
    if(p.size() > MAX_LEN) {
        Log::l() << "path too long (>" << MAX_LEN << ")" << std::endl;
        return false;
    }
    if(!this->m_lock.path(p.data() + std::string(".lock"))) {
        Log::l() << "socket already locked" << std::endl;
        return false;
    }
    const auto id = socket(AF_UNIX, SOCK_STREAM, 0);
    if(id == -1)
        return Log::perror("socket"), false;
    if(fcntl(id, F_SETFL, O_NONBLOCK) == -1)
        return Log::perror("fcntl"), false;
    sockaddr_un addr = {};
    std::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, p.data(), MAX_LEN);
    const auto *addr_dp = reinterpret_cast<const sockaddr*>(&addr);
    if(bind(id, addr_dp, sizeof(addr)) == -1) {
        if(errno != EADDRINUSE)
            return Log::perror("bind"), false;
        if(unlink(p.data()) == -1)
            return Log::perror("unlink"), false;
        if(bind(id, addr_dp, sizeof(addr)) == -1)
            return Log::perror("bind"), false;
    }
    if(listen(id, BACKLOG) == -1)
        return Log::perror("listen"), false;
    this->fd = id;
    this->path = p;
    return true;
}

bool Socket::process(std::string *buffer) {
    return this->fd == -1 ? true : this->accept() && this->recv(buffer);
}

bool Socket::accept() {
    NNGN_LOG_CONTEXT_CF(Socket);
    for(;;) {
        const auto id = ::accept(this->fd, nullptr, nullptr);
        if(id == -1) {
            if(errno == EWOULDBLOCK)
                break;
            return Log::perror("accept"), false;
        }
        if(fcntl(id, F_SETFL, O_CLOEXEC | O_NONBLOCK) == -1) {
            Log::perror("fcntl");
            if(close(id) == -1)
                return Log::perror("close"), false;
        }
        for(auto &x : *to_poll_data(&this->poll_data)) {
            if(x.fd != -1)
                continue;
            x.fd = id;
            break;
        }
    }
    return true;
}

bool Socket::recv(std::string *buffer) {
    NNGN_LOG_CONTEXT_CF(Socket);
    auto &v = *to_poll_data(&this->poll_data);
    switch(poll(v.data(), BACKLOG, 0)) {
    case 0: return true;
    case -1: return Log::perror("poll"), false;
    }
    constexpr auto MAX_LEN = 1024u * 1024u - 1u;
    buffer->resize(MAX_LEN + 1);
    auto *it = std::find_if(
        begin(v), end(v), [](const auto &x) { return x.revents; });
    assert(it != end(v));
    const auto close = [](auto *i) {
        if(::close(std::exchange(*i, -1)) == -1)
            return Log::perror("close"), false;
        return true;
    };
    if(it->revents & POLLERR) {
        Log::l() << "poll: err\n";
        return close(&it->fd);
    }
    if(it->revents & POLLHUP && !(it->revents & POLLIN)) {
        Log::l() << "poll: hup\n";
        return close(&it->fd);
    }
    assert(it->revents & POLLIN);
    size_t size = 0;
    for(;;) {
        const auto n = read(it->fd, buffer->data() + size, MAX_LEN - size);
        switch(n) {
        case -1:
            if(errno != EWOULDBLOCK)
                return Log::perror("read"), false;
            goto end;
        case 0:
            if(!close(&it->fd))
                return false;
            goto end;
        default:
            size += static_cast<size_t>(n);
            break;
        }
    }
end:
    const auto usize = static_cast<size_t>(size);
    buffer->resize(usize + 1);
    (*buffer)[usize] = '\0';
    return true;
}

}

#endif
