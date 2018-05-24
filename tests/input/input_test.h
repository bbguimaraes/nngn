#ifndef NNGN_TEST_INPUT_INPUT_H
#define NNGN_TEST_INPUT_INPUT_H

#include <QTest>

class InputTest : public QObject {
    Q_OBJECT
private slots:
    void get_keys();
    void get_keys_override();
    void override_keys();
    void register_callback_data();
    void register_callback();
    void register_binding_wrong_key_data();
    void register_binding_wrong_key();
    void register_binding_data();
    void register_binding();
};

#endif
