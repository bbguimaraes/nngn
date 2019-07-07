#ifndef NNGN_TEST_FLAGS_H
#define NNGN_TEST_FLAGS_H

#include <QTest>

class FlagsTest : public QObject {
    Q_OBJECT
private slots:
    void constructor(void);
    void operator_bool(void);
    void is_set(void);
    void set(void);
    void set_bool(void);
    void clear(void);
    void check_and_clear(void);
    void exclusive_or(void);
    void minus(void);
    void comp(void);
};

#endif
