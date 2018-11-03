#include "utils.h"

#include <cstring>
#include <fstream>
#include <type_traits>

#include "log.h"

namespace {

template<typename T>
concept byte_range = nngn::byte_pointer<typename T::pointer>;

bool read(std::string_view filename, byte_range auto *v) {
    NNGN_LOG_CONTEXT(filename.data());
    std::ifstream in(filename.data(), std::ios::ate | std::ios::binary);
    if(!in)
        return nngn::Log::perror(), false;
    const auto n = static_cast<std::streamsize>(in.tellg());
    in.seekg(0);
    v->resize(static_cast<std::size_t>(n));
    in.read(nngn::byte_cast<char*>(v->data()), n);
    return true;
}

}

namespace nngn {

bool read_file(std::string_view filename, std::string *ret) {
    NNGN_LOG_CONTEXT_F();
    return read(filename, ret);
}

bool read_file(std::string_view filename, std::vector<std::byte> *ret) {
    NNGN_LOG_CONTEXT_F();
    return read(filename, ret);
}

}
