#ifndef NNGN_TEST_BENCH_ENTITY_H
#define NNGN_TEST_BENCH_ENTITY_H

#include <random>

#include <QTest>

#include "collision/colliders.h"
#include "math/camera.h"
#include "render/animation.h"
#include "render/light.h"
#include "render/renderers.h"

class Entities;

class EntityBench : public QObject {
    Q_OBJECT
    std::vector<nngn::SpriteRenderer> sprites = {};
    std::vector<nngn::Camera> cameras = {};
    std::vector<nngn::Animation> animations = {};
    std::vector<nngn::AABBCollider> colliders = {};
    std::vector<nngn::Light> lights = {};
    void clear();
    Entities gen_entities();
    Entities gen_entities_with_components();
    void benchmark_full(Entities &&es);
    void benchmark(Entities &&es);
public:
    EntityBench();
private slots:
    void update_pos();
    void update_pos_components();
    void update_pos_with_holes();
    void update_pos_components_with_holes();
};

#endif
