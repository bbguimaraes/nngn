#ifndef NNGN_TEST_SUN_H
#define NNGN_TEST_SUN_H

#include <QTest>

class SunTest : public QObject {
    Q_OBJECT
private slots:
    void constructor();
    void dir_data();
    void dir();
};

#endif
