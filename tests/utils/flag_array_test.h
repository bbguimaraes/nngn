#ifndef NNGN_TESTS_UTILS_FLAG_ARRAY_H
#define NNGN_TESTS_UTILS_FLAG_ARRAY_H

#include <QTest>

class FlagArrayTest : public QObject {
    Q_OBJECT
private slots:
    void constructor_data();
    void constructor();
    void set();
    void layers();
};

#endif
