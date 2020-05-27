#include <algorithm>
#include <array>
#include <cstring>
#include <utility>

#include <poll.h>
#include <unistd.h>

#include "utils/log.h"

#include "poller.h"

bool Poller::init() {
    if(pipe(this->pipe_fds.data()) == -1) {
        nngn::Log::perror("pipe");
        return false;
    }
    this->poll_fds = {{
        {.fd = -1, .events = POLLIN},
        {.fd = this->pipe_fds[0]},
    }};
    return true;
}

bool Poller::destroy() {
    return std::all_of(
        this->pipe_fds.cbegin(), this->pipe_fds.cend(), [](auto x) {
            if(x == -1 || close(x) != -1)
                return true;
            nngn::Log::perror("close");
            return false;
        });
}

bool Poller::finish() {
    auto &fd = this->pipe_fds[1];
    if(fd != -1 && close(std::exchange(fd, -1)) == -1) {
        nngn::Log::perror("close");
        return false;
    }
    return true;
}

Poller::result Poller::poll(int fd) {
    constexpr int no_timeout = -1;
    int ret;
    auto v = this->poll_fds;
    v[0].fd = fd;
    while(!(ret = ::poll(v.data(), v.size(), no_timeout)));
    if(ret == -1) {
        nngn::Log::perror("close");
        return result::ERR;
    }
    if(v[1].revents)
        return result::CANCELLED;
    if(v[0].revents == POLLHUP)
        return result::DONE;
    return result::READ;
}
