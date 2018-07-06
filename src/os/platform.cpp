#include "platform.h"

#ifndef NNGN_PLATFORM_EMSCRIPTEN
#include <csignal>
#include <cstring>

#include <unistd.h>

#include "utils/log.h"
#endif

namespace {

void init_common(int argc, const char *const *argv) {
    nngn::Platform::argc = argc;
    nngn::Platform::argv = argv;
}

}

namespace nngn {

int Platform::argc = {};
const char *const *Platform::argv = {};
std::filesystem::path Platform::src_dir = {};

#ifdef NNGN_PLATFORM_EMSCRIPTEN
#include <emscripten.h>

bool Platform::init(int argc_, const char *const *argv_)
    { init_common(argc_, argv_); return true; }

int Platform::loop(int (*loop)()) {
    struct FP { int (*p)(); } fp{loop};
    emscripten_set_main_loop_arg(
        [](void *p) {
            if(static_cast<FP*>(p)->p() != -1)
                emscripten_cancel_main_loop();
        }, &fp, 0, 1);
    return 0;
}
#else
#ifdef HAVE_SIGNAL
bool Platform::init(int argc_, const char *const *argv_) {
    NNGN_LOG_CONTEXT_CF(Platform);
    init_common(argc_, argv_);
    const auto ign = SIG_IGN, err = SIG_ERR;
    const auto *const ign_p = reinterpret_cast<const sighandler_t*>(&ign);
    const auto *const err_p = reinterpret_cast<const sighandler_t*>(&err);
    if(std::signal(SIGPIPE, *ign_p) == *err_p)
        return Log::perror("signal(SIGPIPE, SIG_IGN)"), false;
    return true;
}
#else
bool Platform::init(int argc_, const char *const *argv_)
    { init_common(argc_, argv_); return true; }
#endif

int Platform::loop(int (*loop)()) {
    int ret = {};
    while((ret = loop()) == -1);
    return ret;
}
#endif

}
