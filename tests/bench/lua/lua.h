#ifndef NNGN_TEST_BENCH_LUA_LUA_H
#define NNGN_TEST_BENCH_LUA_LUA_H

#ifndef NNGN_BENCH_LUA_N
// Limited by Lua's maximum stack size (see `LUAI_MAXSTACK` and `EXTRA_STACK`,
// typically 1'000'000 and 5 respectively).
#define NNGN_BENCH_LUA_N (1'000'000 - 5 - 1)
#endif

#include <QTest>

struct lua_State;

class LuaBench : public QObject {
    Q_OBJECT
    lua_State *L = nullptr;
private slots:
    void initTestCase(void);
    void cleanupTestCase(void);
    void base(void);
    void access_stack(void);
    void access_registry(void);
    void access_registry_raw(void);
    void access_global(void);
    void push_stack(void);
    void push_registry(void);
    void push_global(void);
};

#endif
