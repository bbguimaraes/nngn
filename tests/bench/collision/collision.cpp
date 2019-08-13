#include "collision.h"

#include "timing/timing.h"

#include "tests/tests.h"

static constexpr size_t N = NNGN_BENCH_N_COLLIDERS;
static constexpr nngn::vec2 bl = {-.5, -.5}, tr = {.5, .5};

nngn::vec3 CollisionBench::rnd()
    { return {this->pos_dist(this->mt), this->pos_dist(this->mt), 0}; }

nngn::vec3 CollisionBench::rnd_sparse() {
    return {
        this->pos_sparse_dist(this->mt),
        this->pos_sparse_dist(this->mt), 0};
}

void CollisionBench::aabb_benchmark() {
    constexpr size_t max = 256 * N;
    auto c = this->make_colliders();
    c.set_max_colliders(N);
    c.set_max_collisions(max);
    for(size_t i = 0; i < N; ++i) {
        const auto pos = this->rnd().xy();
        c.add(nngn::AABBCollider(pos + bl, pos + ::tr));
    }
    QBENCHMARK { QVERIFY(c.check_collisions(nngn::Timing{})); }
    QVERIFY(c.collisions().size() < max);
}

void CollisionBench::aabb_sparse_benchmark() {
    constexpr size_t max = 16;
    auto c = this->make_colliders();
    c.set_max_colliders(N);
    c.set_max_collisions(max);
    for(size_t i = 0; i < N; ++i) {
        const auto pos = this->rnd_sparse().xy();
        c.add(nngn::AABBCollider(pos + bl, pos + ::tr));
    }
    QBENCHMARK { QVERIFY(c.check_collisions(nngn::Timing{})); }
    QVERIFY(c.collisions().size() < max);
}

void CollisionBench::bb_benchmark() {
    constexpr size_t max = 256 * N;
    auto c = this->make_colliders();
    c.set_max_colliders(N);
    c.set_max_collisions(max);
    for(size_t i = 0; i < N; ++i) {
        const auto pos = this->rnd().xy();
        c.add(nngn::BBCollider(
            pos + bl, pos + ::tr,
            this->rot_dist(this->mt),
            this->rot_dist(this->mt)));
    }
    QBENCHMARK { QVERIFY(c.check_collisions(nngn::Timing{})); }
    QVERIFY(c.collisions().size() < max);
}

void CollisionBench::bb_sparse_benchmark() {
    constexpr size_t max = 16;
    auto c = this->make_colliders();
    c.set_max_colliders(N);
    c.set_max_collisions(max);
    for(size_t i = 0; i < N; ++i) {
        const auto pos = this->rnd_sparse().xy();
        c.add(nngn::BBCollider(
            pos + bl, pos + ::tr,
            this->rot_dist(this->mt),
            this->rot_dist(this->mt)));
    }
    QBENCHMARK { QVERIFY(c.check_collisions(nngn::Timing{})); }
    QVERIFY(c.collisions().size() < max);
}

void CollisionBench::sphere_benchmark() {
    constexpr size_t max = 256 * N;
    auto c = this->make_colliders();
    c.set_max_colliders(N);
    c.set_max_collisions(max);
    for(size_t i = 0; i < N; ++i)
        c.add(nngn::SphereCollider(this->rnd(), .5));
    QBENCHMARK { QVERIFY(c.check_collisions(nngn::Timing{})); }
    QVERIFY(c.collisions().size() < max);
}

void CollisionBench::sphere_sparse_benchmark() {
    constexpr size_t max = 16;
    auto c = this->make_colliders();
    c.set_max_colliders(N);
    c.set_max_collisions(max);
    for(size_t i = 0; i < N; ++i)
        c.add(nngn::SphereCollider(this->rnd_sparse(), .5));
    QBENCHMARK { QVERIFY(c.check_collisions(nngn::Timing{})); }
    QVERIFY(c.collisions().size() < max);
}
