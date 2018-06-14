#ifndef NNGN_TIMING_ADHOC_H
#define NNGN_TIMING_ADHOC_H

#include <string_view>
#include <vector>

#include "timing.h"

namespace nngn {

struct AdHocTimer {
    struct Step { std::string_view name = {}; Timing::duration t = {}; };
    Timing::time_point last = Timing::clock::now();
    std::vector<Step> steps = {};
    void add(std::string_view name);
    void end();
    void log();
};

}

#endif
