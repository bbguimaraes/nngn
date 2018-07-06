#ifndef NNGN_TESTS_LUA_TABLE_TEST_H
#define NNGN_TESTS_LUA_TABLE_TEST_H

#include <QTest>

class TableTest : public QObject {
    Q_OBJECT
private slots:
    void global(void);
    void proxy(void);
    void proxy_multi(void);
    void user_type(void);
};

#endif
