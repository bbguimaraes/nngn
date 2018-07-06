#ifndef NNGN_TESTS_LUA_STATE_H
#define NNGN_TESTS_LUA_STATE_H

#include <QTest>

#include "lua/state.h"

class StateTest : public QObject {
    Q_OBJECT
    nngn::lua::state lua = {};
private slots:
    void init(void);
    void register_all(void);
    void traceback(void);
};

#endif
