#ifndef NNGN_TEST_COLLISION_H
#define NNGN_TEST_COLLISION_H

#include <QTest>

#include "collision/collision.h"

class CollisionTest : public QObject {
    Q_OBJECT
private slots:
    void aabb_collision_data();
    void aabb_collision();
    void bb_collision_data();
    void bb_collision();
    void sphere_sphere_collision_data();
    void sphere_sphere_collision();
    // TODO
//    void plane_collision_data();
//    void plane_collision();
    void plane_sphere_collision_data();
    void plane_sphere_collision();
protected:
    nngn::Colliders colliders = {};
};

#endif
