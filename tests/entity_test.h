#ifndef NNGN_TEST_ENTITY_H
#define NNGN_TEST_ENTITY_H

#include <QTest>

class EntityTest : public QObject {
    Q_OBJECT
private slots:
    void max_v_data();
    void max_v();
    void add_remove();
};

#endif
