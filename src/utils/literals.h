#ifndef NNGN_UTILS_LITERALS_H
#define NNGN_UTILS_LITERALS_H

namespace nngn::literals {

inline std::size_t operator ""_z(unsigned long long int i)
    { return static_cast<std::size_t>(i); }
inline std::ptrdiff_t operator ""_t(unsigned long long int i)
    { return static_cast<std::ptrdiff_t>(i); }

}

#endif
