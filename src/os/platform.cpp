#include "platform.h"

#ifdef NNGN_PLATFORM_EMSCRIPTEN
#include <emscripten.h>
#else
#include <csignal>
#include <cstring>
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

bool Platform::init(int argc_, const char *const *argv_) {
    NNGN_LOG_CONTEXT_CF(Platform);
    init_common(argc_, argv_);
#if defined(HAVE_SIGNAL) && !defined(NNGN_PLATFORM_EMSCRIPTEN)
    if constexpr(constexpr int s = Platform::sig_pipe) {
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
