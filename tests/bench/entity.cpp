#include "entity.h"

#include <algorithm>
#include <functional>
#include <random>

#include "../../src/entity.h"

#include "timing/timing.h"

constexpr std::size_t N = 1u << 20;
constexpr std::size_t N_HOLES = 1u << 10;

namespace {

auto gen = std::mt19937{std::random_device{}()};
auto dist = std::uniform_real_distribution<float>{};

template<typename ...Ts>
Entities gen_entities(Ts &&...fs) {
    Entities es = {};
    es.set_max(N + 1);
    for(size_t i = 0; i < N; ++i) {
        auto *const e = es.add();
        (fs(e), ...);
    }
    return es;
}

void remove_rnd(Entities *es) {
    const auto begin = es->begin(), end = es->end();
    auto i_dist = std::uniform_int_distribution(
        std::ptrdiff_t{}, static_cast<std::ptrdiff_t>(N - 1));
    for(std::size_t i = 0; i < N_HOLES; ++i)
        if(auto e = begin + i_dist(gen); e->alive())
            es->remove(&*e);
    const auto alive = static_cast<std::size_t>(
        std::count_if(begin + 1, end, std::mem_fn(&Entity::alive)));
    QVERIFY(N - alive >= N_HOLES / 2);
}

void rnd_pos(Entity *e) {
    auto r = [] { return dist(gen); };
    e->set_pos({r(), r(), r()});
    e->set_vel({r(), r(), r()});
    e->a = {r(), r(), r()};
}

bool check_alive(const Entities &es) {
    return std::all_of(es.cbegin() + 1, es.cend(), std::mem_fn(&Entity::alive));
}

}

EntityBench::EntityBench() {
    this->sprites.reserve(N);
    this->cameras.reserve(N);
    this->animations.reserve(N);
}

void EntityBench::clear() {
    this->sprites.clear();
    this->cameras.clear();
    this->animations.clear();
}

Entities EntityBench::gen_entities() { return ::gen_entities(rnd_pos); }

Entities EntityBench::gen_entities_with_components() {
    this->clear();
    return ::gen_entities(
        rnd_pos,
        [this](auto *e) { e->set_renderer(&this->sprites.emplace_back()); },
        [this](auto *e) { e->set_camera(&this->cameras.emplace_back()); },
        [this](auto *e)
            { e->set_animation(&this->animations.emplace_back()); });
}

void EntityBench::benchmark_full(Entities &&es) {
    QVERIFY(check_alive(es));
    return this->benchmark(std::forward<Entities>(es));
}

void EntityBench::benchmark(Entities &&es) {
    nngn::Timing t = {};
    t.dt = std::chrono::milliseconds{16};
    QBENCHMARK { es.update(t); }
}

void EntityBench::update_pos()
    { this->benchmark_full(this->gen_entities()); }
void EntityBench::update_pos_components()
    { this->benchmark_full(this->gen_entities_with_components()); }

void EntityBench::update_pos_with_holes() {
    auto es = this->gen_entities();
    remove_rnd(&es);
    this->benchmark(std::move(es));
}

void EntityBench::update_pos_components_with_holes() {
    auto es = this->gen_entities_with_components();
    remove_rnd(&es);
    this->benchmark(std::move(es));
}

QTEST_MAIN(EntityBench)
