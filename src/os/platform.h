#ifndef NNGN_OS_PLATFORM_H
#define NNGN_OS_PLATFORM_H

#include <csignal>
#include <filesystem>
#include <span>

#include "config.h"

#if defined(_WIN64) || defined(__x86_64__)
    #define NNGN_PLATFORM_64BIT
#else
    #define NNGN_PLATFORM_32BIT
#endif

#ifdef __EMSCRIPTEN__
    #define NNGN_PLATFORM_EMSCRIPTEN
    #undef HAVE_TERMIOS_H
    #undef NNGN_PLATFORM_HAS_SOCKETS
#endif

namespace nngn {

struct Platform {
#ifndef NDEBUG
    static constexpr bool debug = true;
#else
    static constexpr bool debug = false;
#endif
#ifdef NNGN_PLATFORM_32BIT
    static constexpr bool m32 = true, m64 = false;
#elif defined(NNGN_PLATFORM_64BIT)
    static constexpr bool m32 = false, m64 = true;
#endif
#ifdef NNGN_PLATFORM_EMSCRIPTEN
    static constexpr bool emscripten = true;
#else
    static constexpr bool emscripten = false;
#endif
#ifdef NNGN_LUA_USE_ALLOC
    static constexpr bool lua_use_alloc = true;
#else
    static constexpr bool lua_use_alloc = false;
#endif
#ifdef NNGN_PLATFORM_HAS_SOCKETS
    static constexpr bool has_sockets = true;
#else
    static constexpr bool has_sockets = false;
#endif
#ifdef NNGN_PLATFORM_HAS_VMA
    static constexpr bool has_vma = true;
#else
    static constexpr bool has_vma = false;
#endif
#ifdef SIGPIPE
    static constexpr int sig_pipe = SIGPIPE;
#else
    static constexpr int sig_pipe = 0;
#endif
    static std::span<const char *const> argv;
    static std::filesystem::path src_dir;
    static bool init(int argc, const char *const *argv);
    static void setenv(const char *name, const char *value);
    static bool set_non_blocking(FILE *f);
    static int loop(int (*loop)(void));
};

}

#endif
