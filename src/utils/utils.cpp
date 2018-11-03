#include <cstring>
#include <fstream>

#include "log.h"
#include "utils.h"

namespace nngn {

bool read_file(std::string_view filename, std::string *ret) {
    NNGN_LOG_CONTEXT("read_file");
    NNGN_LOG_CONTEXT(filename.data());
    std::ifstream in(filename.data(), std::ios::ate | std::ios::binary);
    if(!in)
        return Log::perror(), false;
    const auto n = static_cast<std::streamsize>(in.tellg());
    in.seekg(0);
    ret->resize(static_cast<size_t>(n));
    in.read(ret->data(), n);
    return true;
}

}
