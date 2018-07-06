#ifndef NNGN_TESTS_LUA_STACK_H
#define NNGN_TESTS_LUA_STACK_H

#include <QTest>

#include "lua/state.h"

class StackTest : public QObject {
    Q_OBJECT
    nngn::lua::state lua = {};
private slots:
    void init(void);
    void optional(void);
    void boolean(void);
    void light_user_data(void);
    void integer(void);
    void number(void);
    void enum_(void);
    void cstr(void);
    void string_view(void);
    void string(void);
    void c_fn(void);
    void c_fn_ptr(void);
    void lambda(void);
    void table(void);
    void optional_table(void);
    void user_type(void);
    void user_type_ptr(void);
    void user_type_ref(void);
    void user_type_light(void);
    void user_type_align(void);
    void user_type_destructor(void);
    void tuple(void);
    void error(void);
    void variant(void);
    void vector(void);
    void type(void);
    void remove(void);
    void str(void);
};

#endif
