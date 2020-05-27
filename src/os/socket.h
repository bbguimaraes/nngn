#ifndef NNGN_OS_SOCKET_H
#define NNGN_OS_SOCKET_H

#include <array>
#include <string>
#include <vector>

#include "timing/profile.h"

namespace nngn {

class Socket {
    class lock {
        int fd = -1;
        std::string m_path = {};
    public:
        lock() = default;
        lock(const lock&) = delete;
        lock(const lock&&) = delete;
        lock &operator=(const lock&) = delete;
        lock &operator=(const lock&&) = delete;
        ~lock();
        bool path(std::string_view path);
    };
    lock m_lock = {};
    int fd = -1;
    std::vector<std::byte> poll_data = {};
    std::string path = {};
    bool accept();
    bool process(std::string *buffer);
    bool recv(std::string *buffer);
public:
    Socket();
    Socket(const Socket&) = delete;
    Socket(const Socket&&) = delete;
    Socket &operator=(const Socket&) = delete;
    Socket &operator=(const Socket&&) = delete;
    ~Socket();
    bool init(std::string_view path);
    template<typename F> bool process(F f);
};

template<typename F> bool Socket::process(F f) {
    NNGN_PROFILE_CONTEXT(socket);
    std::string buffer;
    if(!this->process(&buffer))
        return false;
    if(!buffer.empty())
        f(std::string_view(buffer));
    return true;
}

}

#endif
