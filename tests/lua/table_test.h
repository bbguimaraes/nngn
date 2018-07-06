#ifndef NNGN_TESTS_LUA_TABLE_TEST_H
#define NNGN_TESTS_LUA_TABLE_TEST_H

#include <QTest>

#include "lua/state.h"

class TableTest : public QObject {
    Q_OBJECT
    nngn::lua::state lua = {};
private slots:
    void init(void);
    void global(void);
    void proxy(void);
    void proxy_multi(void);
    void proxy_default(void);
    void proxy_default_table(void);
    void user_type(void);
    void size(void);
    void iter(void);
    void iter_ipairs(void);
    void iter_empty(void);
    void iter_ipairs_empty(void);
    void iter_stack(void);
    void iter_ipairs_stack(void);
    void iter_nested(void);
};

#endif
