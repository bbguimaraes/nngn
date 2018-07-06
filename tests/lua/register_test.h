#ifndef NNGN_TESTS_LUA_REGISTER_H
#define NNGN_TESTS_LUA_REGISTER_H

#include <QTest>

#include "lua/state.h"
#include "lua/table.h"

class RegisterTest : public QObject {
    Q_OBJECT
    nngn::lua::state lua = {};
    nngn::lua::global_table g = {};
    nngn::lua::table mt = {};
private slots:
    void init(void);
    void cleanup(void);
    void user_type(void);
    void meta_table(void);
    void accessor(void);
    void value_accessor(void);
    void member_fn(void);
};

#endif
