#ifndef NNGN_TEST_COLLISION_H
#define NNGN_TEST_COLLISION_H

#include <QTest>

#include "collision/collision.h"

class CollisionTest : public QObject {
    Q_OBJECT
private slots:
    void aabb_collision_data();
    void aabb_collision();
protected:
    nngn::Colliders colliders = {};
};

#endif
