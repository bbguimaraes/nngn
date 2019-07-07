#ifndef NNGN_TEST_FLAGS_H
#define NNGN_TEST_FLAGS_H

#include <QTest>

class FlagsTest : public QObject {
    Q_OBJECT
private slots:
    void constructor();
    void operator_bool();
    void is_set();
    void set();
    void set_bool();
    void clear();
    void check_and_clear();
    void exclusive_or();
    void minus();
    void flip();
};

#endif
