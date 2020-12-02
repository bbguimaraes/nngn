#ifndef IMPERO_TESTS_TESTS_H
#define IMPERO_TESTS_TESTS_H

#include <string_view>

#include <QTest>

namespace std {

char *toString(string s)
    { return QTest::toString(QString::fromUtf8(s.data(), s.size())); }
char *toString(string_view s)
    { return QTest::toString(QString::fromUtf8(s.data(), s.size())); }

}

#endif
