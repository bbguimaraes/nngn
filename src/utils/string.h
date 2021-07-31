#ifndef NNGN_UTILS_STRING_H
#define NNGN_UTILS_STRING_H

#include <sstream>
#include <string>

#include "utils.h"

namespace nngn {

std::string fmt(auto &&...args) {
    std::stringstream s = {};
    (s << ... << FWD(args));
    return s.str();
}

}

#endif
