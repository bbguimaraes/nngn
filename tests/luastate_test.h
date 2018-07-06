#ifndef NNGN_TEST_LUASTATE_H
#define NNGN_TEST_LUASTATE_H

#include <QTest>

class LuastateTest : public QObject {
    Q_OBJECT
private slots:
    void luastate();
    void init_state();
    void register_all();
    void traceback();
};

#endif
