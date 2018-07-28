#ifndef NNGN_TEST_MAP_H
#define NNGN_TEST_MAP_H

#include <QTest>

class MapTest : public QObject {
    Q_OBJECT
private slots:
    void load_tiles();
    void gen();
};

#endif
