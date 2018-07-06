#ifndef NNGN_OS_PLATFORM_H
#define NNGN_OS_PLATFORM_H

#include <csignal>
#include <filesystem>
#include <span>

#include "config.h"

#ifdef __EMSCRIPTEN__
    #define NNGN_PLATFORM_EMSCRIPTEN
#endif

namespace nngn {

struct Platform {
#ifndef NDEBUG
    static constexpr bool debug = true;
#else
    static constexpr bool debug = false;
#endif
#ifdef NNGN_PLATFORM_EMSCRIPTEN
    static constexpr bool emscripten = true;
#else
    static constexpr bool emscripten = false;
#endif
#ifdef SIGPIPE
    static constexpr int sig_pipe = SIGPIPE;
#else
    static constexpr int sig_pipe = 0;
#endif
    static std::span<const char *const> argv;
    static std::filesystem::path src_dir;
    static bool init(int argc, const char *const *argv);
    static int loop(int (*loop)(void));
};

}

#endif
