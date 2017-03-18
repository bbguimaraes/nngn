#ifndef NNGN_OS_PLATFORM_H
#define NNGN_OS_PLATFORM_H

#include <filesystem>

#include "config.h"

namespace nngn {

struct Platform {
#ifndef NDEBUG
    static constexpr bool debug = true;
#else
    static constexpr bool debug = false;
#endif
    static int argc;
    static const char *const *argv;
    static std::filesystem::path src_dir;
    static bool init(int argc, const char *const *argv);
    static int loop(int (*loop)());
};

}

#endif
