#include <csignal>
#include <cstring>

#include <unistd.h>

#include "platform.h"

#include "utils/log.h"

namespace {

void init_common(int argc, const char *const *argv) {
    nngn::Platform::argv = {argv, static_cast<std::size_t>(argc)};
}

}

namespace nngn {

std::span<const char *const> Platform::argv = {};
std::filesystem::path Platform::src_dir = {};

bool Platform::init(int argc_, const char *const *argv_) {
    NNGN_LOG_CONTEXT_CF(Platform);
    init_common(argc_, argv_);
#ifdef HAVE_SIGNAL
    if constexpr(constexpr int s = Platform::sig_pipe; s != 0) {
        const auto ign = SIG_IGN, err = SIG_ERR;
        using sig_t = void(*)(int);
        const auto *const ign_p = reinterpret_cast<const sig_t*>(&ign);
        const auto *const err_p = reinterpret_cast<const sig_t*>(&err);
        if(std::signal(s, *ign_p) == *err_p)
            return Log::perror("signal(SIGPIPE, SIG_IGN)"), false;
    }
#endif
    return true;
}

int Platform::loop(int (*loop)()) {
    int ret = {};
    while((ret = loop()) == -1);
    return ret;
}

}
