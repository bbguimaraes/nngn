#ifndef NNGN_TEST_BENCH_COLLISION_H
#define NNGN_TEST_BENCH_COLLISION_H

#include <random>

#include <QTest>

#include "collision/collision.h"
#include "math/vec3.h"

#ifndef NNGN_BENCH_N_COLLIDERS
#define NNGN_BENCH_N_COLLIDERS 1u << 10
#endif

class CollisionBench : public QObject {
    Q_OBJECT
    std::mt19937 mt = {};
    std::uniform_real_distribution<float> pos_dist, pos_sparse_dist, rot_dist;
    nngn::vec3 rnd();
    nngn::vec3 rnd_sparse();
protected:
    virtual nngn::Colliders make_colliders() const = 0;
public:
    CollisionBench() :
        pos_dist(0, 16),
        pos_sparse_dist(0, 1u << 16),
        rot_dist(-1, 1) {}
private slots:
    void aabb_benchmark();
    void aabb_sparse_benchmark();
    void bb_benchmark();
    void bb_sparse_benchmark();
    void sphere_benchmark();
    void sphere_sparse_benchmark();
};

#endif
