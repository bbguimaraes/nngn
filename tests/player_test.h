#ifndef NNGN_TEST_PLAYER_H
#define NNGN_TEST_PLAYER_H

#include <QTest>

class PlayerTest : public QObject {
    Q_OBJECT
private slots:
    void add();
    void remove_data();
    void remove();
};

#endif
