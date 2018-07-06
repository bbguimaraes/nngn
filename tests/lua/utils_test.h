#ifndef NNGN_TESTS_LUA_UTILS_H
#define NNGN_TESTS_LUA_UTILS_H

#include <QTest>

class UtilsTest : public QObject {
    Q_OBJECT
private slots:
    void defer_pop(void);
    void stack_mark(void);
};

#endif
