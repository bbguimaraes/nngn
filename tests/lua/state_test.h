#ifndef NNGN_TESTS_LUA_STATE_H
#define NNGN_TESTS_LUA_STATE_H

#include <QTest>

class StateTest : public QObject {
    Q_OBJECT
private slots:
    void constructor(void);
    void init(void);
    void register_all(void);
    void traceback(void);
};

#endif
