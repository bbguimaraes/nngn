#ifndef NNGN_TOOLS_UTILS_H
#define NNGN_TOOLS_UTILS_H

#include <QRegularExpression>
#include <QStringView>

namespace nngn::literals {

inline auto operator""_qs(const char *p, std::size_t n) {
    return QString::fromUtf8(p, static_cast<int>(n));
}

inline auto operator""_qre(const char *p, std::size_t n) {
    return QRegularExpression(QString::fromUtf8(p, static_cast<int>(n)));
}

}

#endif
