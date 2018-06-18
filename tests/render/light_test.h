#ifndef NNGN_TEST_LIGHT_H
#define NNGN_TEST_LIGHT_H

#include <QTest>

class LightTest : public QObject {
    Q_OBJECT
private slots:
    void add_light();
    void remove_light();
    void remove_add_light();
    void max_lights();
    void ubo_ambient_light();
    void ubo_n();
    void update_dir();
    void update_point();
    void updated();
    void entity();
};

#endif
