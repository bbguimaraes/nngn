#ifndef NNGN_SCRIPTS_WORKER_H
#define NNGN_SCRIPTS_WORKER_H

#include <array>
#include <poll.h>

class Poller {
    std::array<int, 2> pipe_fds = {-1, -1};
    std::array<pollfd, 2> poll_fds = {};
public:
    enum class result : uint8_t { ERR, DONE, CANCELLED, READ };
    bool init();
    bool destroy();
    bool finish();
    result poll(int fd);
};

#endif
