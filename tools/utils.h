#ifndef NNGN_TOOLS_UTILS_H
#define NNGN_TOOLS_UTILS_H

#include <QRegularExpression>
#include <QStringView>

namespace nngn {

inline QString qstring_from_view(std::string_view v) {
    return QString::fromUtf8(v.data(), static_cast<int>(v.size()));
}

}

namespace nngn::literals {

inline auto operator""_qs(const char *p, std::size_t n) {
    return QString::fromUtf8(p, static_cast<int>(n));
}

inline auto operator""_qre(const char *p, std::size_t n) {
    return QRegularExpression(QString::fromUtf8(p, static_cast<int>(n)));
}

}

#endif
