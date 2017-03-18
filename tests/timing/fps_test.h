#ifndef NNGN_TEST_FPS_H
#define NNGN_TEST_FPS_H

#include <QTest>

class FpsTest : public QObject {
    Q_OBJECT
private slots:
    void constructor();
    void sec();
    void avg();
};

#endif
