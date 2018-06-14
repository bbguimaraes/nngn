#ifndef NNGN_TEST_ENTITY_H
#define NNGN_TEST_ENTITY_H

#include <QTest>

class EntityTest : public QObject {
    Q_OBJECT
private slots:
    void add_remove();
};

#endif
