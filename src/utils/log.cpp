#include "log.h"

#include <cassert>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <span>

#include "utils.h"

namespace {

char *strerror(int e, [[maybe_unused]] std::span<char> s) {
#ifdef HAVE_STRERROR_R
    const auto ret = strerror_r(e, s.data(), s.size());
    constexpr bool posix = std::is_same_v<decltype(ret), int>;
    if constexpr(!posix)
        return nngn::cast<char*>(ret);
    return s.data();
#else
    return ::strerror(e);
#endif
}

}

namespace nngn {

std::ostream &operator<<(std::ostream &os, Log::record r) {
    auto &[n0, n1] = r;
    return n1 ? os << n0 << "::" << n1 : os << n0;
}

std::ostream *Log::stream =
    []{ const auto init = std::ios_base::Init(); return &std::cerr; }();
thread_local uint8_t Log::depth = 0;
thread_local std::array<Log::record, Log::MAX_DEPTH> Log::stack;

Log::context::context(const char *name0, const char *name1) {
    const auto r = Log::record(name0, name1);
    if(const auto i = Log::depth++; i < MAX_DEPTH) {
        Log::stack[i] = r;
        return;
    }
    const auto d = static_cast<unsigned>(Log::MAX_DEPTH);
    Log::l()
        << "Log::context(" << r << "): max depth reached (" << d << ")"
        << std::endl;
}

Log::context::~context()
    { [[maybe_unused]] const auto i = Log::depth--; assert(i); }
std::ostream *Log::set(std::ostream *l)
    { return std::exchange(Log::stream, l); }

std::ostream &Log::l() {
    for(size_t i = 0; i < std::min(Log::depth, Log::MAX_DEPTH); ++i)
        *Log::stream << Log::stack[i] << ": ";
    return *Log::stream;
}

void Log::perror(const char *s) {
    char buffer[1024];
    const char *const err = strerror(errno, buffer);
    auto &l = Log::l();
    if(s)
        l << s << ": ";
    l << err << '\n';
    errno = 0;
}

}
