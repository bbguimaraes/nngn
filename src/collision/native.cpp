#include <algorithm>

#include "collision/collision.h"
#include "math/math.h"
#include "timing/profile.h"
#include "utils/log.h"

using nngn::Colliders;
using Input = Colliders::Backend::Input;
using Output = Colliders::Backend::Output;
using nngn::AABBCollider;

namespace {

void check_aabb(std::span<AABBCollider> aabb, Output *output);
bool check_bb_fast(const AABBCollider &c0, const AABBCollider &c1);
constexpr float overlap(float min0, float max0, float min1, float max1);
bool float_eq_zero(float f);
template<typename T, typename U>
bool add_collision(
    T *c0, U *c1, const nngn::vec3 &v,
    std::vector<nngn::Collision> *output);

class NativeBackend final : public Colliders::Backend {
    bool check(const nngn::Timing &t, Input *input, Output *output) final;
};

bool NativeBackend::check(
    const nngn::Timing&, Input *input, Output *output
) {
    { NNGN_STATS_CONTEXT(Colliders, &output->stats.counters); }
    check_aabb(input->aabb, output);
    auto &v = *nngn::Stats::u64_data<Colliders>();
    for(std::size_t i = 0, n = v.size(); i < n; i += 4)
        v[i + 1] = v[i + 2] = v[i];
    return true;
}

void check_aabb(std::span<AABBCollider> aabb, Output *output) {
    { NNGN_STATS_CONTEXT(Colliders, &output->stats.aabb_copy); }
    { NNGN_STATS_CONTEXT(Colliders, &output->stats.aabb_exec_barrier); }
    NNGN_STATS_CONTEXT(Colliders, &output->stats.aabb_exec);
    for(auto i0 = begin(aabb), e = end(aabb); i0 != e; ++i0) {
        auto &c0 = *i0;
        for(auto i1 = i0 + 1; i1 != e; ++i1) {
            auto &c1 = *i1;
            if(!check_bb_fast(c0, c1))
                continue;
            const float xoverlap = overlap(c0.bl.x, c0.tr.x, c1.bl.x, c1.tr.x);
            if(float_eq_zero(xoverlap))
                continue;
            const float yoverlap = overlap(c0.bl.y, c0.tr.y, c1.bl.y, c1.tr.y);
            if(float_eq_zero(yoverlap))
                continue;
            const auto v = std::fabs(xoverlap) <= std::fabs(yoverlap)
                ? nngn::vec3(-xoverlap, 0, 0)
                : nngn::vec3(0, -yoverlap, 0);
            if(!add_collision(&c0, &c1, v, &output->collisions))
                return;
        }
    }
}

bool check_bb_fast(const AABBCollider &c0, const AABBCollider &c1) {
    return nngn::Math::length2(c1.center - c0.center)
        < (c0.radius + c1.radius) * (c0.radius + c1.radius);
}

constexpr float overlap(float min0, float max0, float min1, float max1) {
    return (min1 > max0 || max1 < min0) ? 0.0f
        : (max0 > max1) ? (min0 - max1)
        : (max0 - min1);
}

bool float_eq_zero(float f)
    { return std::fabs(f) <= std::numeric_limits<float>::epsilon() * 2; }

template<typename T, typename U>
bool add_collision(
    T *c0, U *c1, const nngn::vec3 &v,
    std::vector<nngn::Collision> *out)
{
    c0->flags.set(nngn::Collider::Flag::COLLIDING);
    c1->flags.set(nngn::Collider::Flag::COLLIDING);
    if((std::isinf(c0->m) && std::isinf(c1->m)))
        return true;
    if(out->size() == out->capacity()) {
        nngn::Log::l() << "too many collisions\n";
        return false;
    }
    out->push_back({
        .entity0 = c0->entity,
        .entity1 = c1->entity,
        .mass0 = c0->m,
        .mass1 = c1->m,
        .flags0 = c0->flags,
        .flags1 = c1->flags,
        .force = v,
    });
    return true;
}

}

namespace nngn {

auto Colliders::native_backend() -> std::unique_ptr<Backend>
    { return std::make_unique<NativeBackend>(); }

}
