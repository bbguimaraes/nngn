#ifndef NNGN_TESTS_LUA_FUNCTION_H
#define NNGN_TESTS_LUA_FUNCTION_H

#include <QTest>

#include "lua/state.h"

class FunctionTest : public QObject {
    Q_OBJECT
    nngn::lua::state lua = {};
private slots:
    void init(void);
    void c_fn(void);
    void lua_fn(void);
    void c_lua_fn(void);
    void call(void);
    void state(void);
    void pcall(void);
};

#endif
