#ifndef NNGN_TEST_BENCH_ENTITY_H
#define NNGN_TEST_BENCH_ENTITY_H

#include <random>

#include <QTest>

#include "render/renderers.h"

class Entities;

class EntityBench : public QObject {
    Q_OBJECT
    std::vector<nngn::SpriteRenderer> sprites = {};
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
