#ifndef NNGN_UTILS_REGEXP_H
#define NNGN_UTILS_REGEXP_H

#include <regex>

namespace nngn {

inline std::string regexp_replace(
    std::string &&s, const std::regex &r, const char *fmt)
{
    const auto b = begin(s), e = end(s);
    s.erase(std::regex_replace(b, b, e, r, fmt), e);
    return std::move(s);
}

inline std::string regexp_replace(
    std::string_view s, const std::regex &r, const char *fmt)
{
    return regexp_replace(std::string{s}, r, fmt);
}

}

#endif
