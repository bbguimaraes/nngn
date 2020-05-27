#include "platform.h"

#ifdef NNGN_PLATFORM_EMSCRIPTEN
#include <emscripten.h>
#else
#include <csignal>
#include <cstdlib>
#include <cstring>
#endif

#if defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L
#include <fcntl.h>
#endif

#include "utils/log.h"

namespace {

void init_common(int argc, const char *const *argv) {
    nngn::Platform::argv = {argv, static_cast<std::size_t>(argc)};
}

}

namespace nngn {

std::span<const char *const> Platform::argv = {};
std::filesystem::path Platform::src_dir = {};

void Platform::setenv(
    [[maybe_unused]] const char *name,
    [[maybe_unused]] const char *value
) {
#ifdef HAVE_SETENV
    ::setenv(name, value, true);
#else
    NNGN_LOG_CONTEXT_F();
    Log::l() << "not supported\n";
#endif
}

bool Platform::init(int argc_, const char *const *argv_) {
    NNGN_LOG_CONTEXT_CF(Platform);
    init_common(argc_, argv_);
#if defined(HAVE_SIGNAL) && !defined(NNGN_PLATFORM_EMSCRIPTEN)
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

bool Platform::set_non_blocking([[maybe_unused]] FILE *f) {
    NNGN_LOG_CONTEXT_CF(Platform);
#if defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L
    const int fd = fileno(f);
    if(fd == -1)
        return Log::perror("fileno"), false;
    if(fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
        return Log::perror("fcntl"), false;
    return true;
#else
    Log::l() << "compiled without POSIX support\n";
    return false;
#endif
}

int Platform::loop(int (*loop)(void)) {
#ifdef NNGN_PLATFORM_EMSCRIPTEN
    struct FP { int (*p)(); } fp{loop};
    emscripten_set_main_loop_arg(
        [](void *p) {
            if(static_cast<FP*>(p)->p() != -1)
                emscripten_cancel_main_loop();
        }, &fp, 0, 1);
    return 0;
#else
    int ret = {};
    while((ret = loop()) == -1);
    return ret;
#endif
}

}
