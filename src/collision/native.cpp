#include <algorithm>

#include "collision/collision.h"
#include "math/math.h"
#include "timing/profile.h"
#include "utils/log.h"

using nngn::Colliders;
using Input = Colliders::Backend::Input;
using Output = Colliders::Backend::Output;
using nngn::AABBCollider;
using nngn::BBCollider;
using nngn::SphereCollider;
using nngn::PlaneCollider;
using nngn::GravityCollider;

namespace {

void check_aabb(std::span<AABBCollider> aabb, Output *output);
void check_bb(std::span<BBCollider> s, Output *output);
void check_sphere(std::span<SphereCollider> s, Output *output);
void check_plane(std::span<PlaneCollider> s, Output *output);
template<typename T> void check_gravity(
    std::span<T> s, std::span<nngn::GravityCollider> gravity,
    Output *output);
void check_aabb_bb(
    std::span<AABBCollider> aabb, std::span<BBCollider> bb,
    Output *output);
void check_aabb_sphere(
    std::span<AABBCollider> aabb, std::span<SphereCollider> sphere,
    Output *output);
void check_bb_sphere(
    std::span<BBCollider> bb, std::span<SphereCollider> sphere,
    Output *output);
void check_sphere_plane(
    std::span<SphereCollider> sphere, std::span<PlaneCollider> plane,
    Output *output);
bool check_bb_fast(const AABBCollider &c0, const AABBCollider &c1);
constexpr float overlap(float min0, float max0, float min1, float max1);
bool float_eq_zero(float f);
constexpr nngn::vec2 rotate(const nngn::vec2 &p, float cos, float sin);
constexpr std::array<nngn::vec2, 4> to_edges(
    const nngn::vec2 &bl, const nngn::vec2 &tr);
inline bool check_bb_common(
    const nngn::vec2 &bl, const nngn::vec2 &tr,
    const std::array<nngn::vec2, 4> &v1, nngn::vec2 *output);
inline bool check_bb_sphere_common(
    const nngn::vec2 &c0, const nngn::vec2 &bl0, const nngn::vec2 &tr0,
    const nngn::vec2 &sc, float sr, nngn::vec2 *output);
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
    check_bb(input->bb, output);
    check_sphere(input->sphere, output);
    check_plane(input->plane, output);
    check_gravity(std::span{input->gravity}, input->gravity, output);
    check_aabb_bb(input->aabb, input->bb, output);
    check_aabb_sphere(input->aabb, input->sphere, output);
    check_bb_sphere(input->bb, input->sphere, output);
    check_sphere_plane(input->sphere, input->plane, output);
    check_gravity(std::span{input->aabb}, input->gravity, output);
    check_gravity(std::span{input->bb}, input->gravity, output);
    check_gravity(std::span{input->sphere}, input->gravity, output);
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

void check_bb(std::span<BBCollider> bb, Output *output) {
    { NNGN_STATS_CONTEXT(Colliders, &output->stats.bb_copy); }
    { NNGN_STATS_CONTEXT(Colliders, &output->stats.bb_exec_barrier); }
    NNGN_STATS_CONTEXT(Colliders, &output->stats.bb_exec);
    for(auto i0 = begin(bb), e = end(bb); i0 != e; ++i0) {
        auto &c0 = *i0;
        const auto rel_bl0 = c0.bl - c0.center, rel_tr0 = c0.tr - c0.center;
        for(auto i1 = i0 + 1; i1 != e; ++i1) {
            auto &c1 = *i1;
            if(!check_bb_fast(c0, c1))
                continue;
            const auto rel_bl1 = c1.bl - c1.center;
            const auto rel_tr1 = c1.tr - c1.center;
            auto edges = to_edges(rel_bl0, rel_tr0);
            for(auto &x : edges)
                x = rotate(
                    rotate(x, c0.cos, c0.sin) + c0.center - c1.center,
                    c1.cos, -c1.sin);
            nngn::vec2 v0 = {};
            if(!check_bb_common(rel_bl1, rel_tr1, edges, &v0))
                continue;
            edges = to_edges(rel_bl1, rel_tr1);
            for(auto &x : edges)
                x = rotate(
                    rotate(x, c1.cos, c1.sin) + c1.center - c0.center,
                    c0.cos, -c0.sin);
            nngn::vec2 v1 = {};
            if(!check_bb_common(rel_bl0, rel_tr0, edges, &v1))
                continue;
            v0 = nngn::Math::length2(v0) <= nngn::Math::length2(v1)
                ? -rotate(v0, c1.cos, c1.sin)
                : rotate(v1, c0.cos, c0.sin);
            if(!add_collision(&c0, &c1, {v0, 0}, &output->collisions))
                return;
        }
    }
}

void check_sphere(std::span<SphereCollider> sphere, Output *output) {
    { NNGN_STATS_CONTEXT(Colliders, &output->stats.sphere_pos); }
    { NNGN_STATS_CONTEXT(Colliders, &output->stats.sphere_vel); }
    { NNGN_STATS_CONTEXT(Colliders, &output->stats.sphere_mass); }
    { NNGN_STATS_CONTEXT(Colliders, &output->stats.sphere_radius); }
    { NNGN_STATS_CONTEXT(Colliders, &output->stats.sphere_grid_count); }
    { NNGN_STATS_CONTEXT(Colliders, &output->stats.sphere_exec_grid_barrier); }
    { NNGN_STATS_CONTEXT(Colliders, &output->stats.sphere_exec_grid); }
    { NNGN_STATS_CONTEXT(Colliders, &output->stats.sphere_exec_barrier); }
    NNGN_STATS_CONTEXT(Colliders, &output->stats.sphere_exec);
    for(auto i0 = begin(sphere), e = end(sphere); i0 != e; ++i0) {
        auto &c0 = *i0;
        for(auto i1 = i0 + 1; i1 != e; ++i1) {
            auto &c1 = *i1;
            const auto d = c0.pos - c1.pos;
            const auto r = c0.r + c1.r;
            const auto l2 = nngn::Math::length2(d);
            if(l2 >= r * r || l2 == 0)
                continue;
            const auto l = std::sqrt(l2);
            const auto v = (r - l) / l * d;
            if(!add_collision(&c0, &c1, v, &output->collisions))
                return;
        }
    }
}

void check_plane(std::span<PlaneCollider>, Output *output) {
    NNGN_STATS_CONTEXT(Colliders, &output->stats.plane);
    // TODO
//    nngn::vec3 v = {};
//    for(auto i0 = begin(plane), e = end(plane); i0 != e; ++i0) {
//        auto &c0 = *i0;
//        for(auto i1 = i0 + 1; i1 != e; ++i1) {
//            auto &c1 = *i1;
//            {
//                const auto &v0 = c0.abcd, v1 = c1.abcd;
//                auto cross = nngn::Math::cross(nngn::vec3(v0), nngn::vec3(v1));
//                if(cross.x == 0 && cross.y == 0 && cross.z == 0)
//                    continue;
//                v = cross;
//                cross = {std::abs(cross.x), std::abs(cross.y), std::abs(cross.z)};
//                const auto max = std::max({cross.x, cross.y, cross.z});
//                if(max == cross.x)
//                    v = {
//                        0,
//                        (v0.z * v1.w - v1.z * v0.w) / cross.x,
//                        (v1.y * v0.w - v0.y * v1.w) / cross.x};
//                else if(max == cross.y)
//                    v = {
//                        (v1.z * v0.w - v0.z * v1.w) / cross.y,
//                        0,
//                        (v0.x * v1.w - v1.x * v0.w) / cross.y};
//                else
//                    v = {
//                        (v0.y * v1.w - v1.y * v0.w) / cross.z,
//                        (v1.x * v0.w - v0.x * v1.w) / cross.z,
//                        0};
//            }
//            if(!add_collision(&c0, &c1, v, &output->collisions))
//                return;
//        }
//    }
}

void check_aabb_bb(
    std::span<AABBCollider> aabb, std::span<BBCollider> bb,
    Output *output
) {
    { NNGN_STATS_CONTEXT(Colliders, &output->stats.aabb_bb_exec_barrier); }
    NNGN_STATS_CONTEXT(Colliders, &output->stats.aabb_bb_exec);
    if(aabb.empty())
        return;
    for(auto &c0 : bb) {
        const auto rel_bl0 = c0.bl - c0.center, rel_tr0 = c0.tr - c0.center;
        for(auto &c1 : aabb) {
            if(!check_bb_fast(c0, c1))
                continue;
            const auto rel_bl1 = c1.bl - c1.center;
            const auto rel_tr1 = c1.tr - c1.center;
            auto edges0 = to_edges(rel_bl0, rel_tr0);
            for(auto &x : edges0)
                x = rotate(x, c0.cos, c0.sin) + c0.center - c1.center;
            nngn::vec2 v0 = {};
            if(!check_bb_common(rel_bl1, rel_tr1, edges0, &v0))
                continue;
            auto edges1 = to_edges(c1.bl, c1.tr);
            for(auto &x : edges1)
                x = rotate(x - c0.center, c0.cos, -c0.sin);
            nngn::vec2 v1 = {};
            if(!check_bb_common(rel_bl0, rel_tr0, edges1, &v1))
                continue;
            v0 = nngn::Math::length2(v0) <= nngn::Math::length2(v1)
                ? -v0 : rotate(v1, c0.cos, c0.sin);
            if(!add_collision(&c0, &c1, {v0, 0}, &output->collisions))
                return;
        }
    }
}

void check_aabb_sphere(
    std::span<AABBCollider> aabb, std::span<SphereCollider> sphere,
    Output *output
) {
    { NNGN_STATS_CONTEXT(Colliders, &output->stats.aabb_sphere_exec_barrier); }
    NNGN_STATS_CONTEXT(Colliders, &output->stats.aabb_sphere_exec);
    if(sphere.empty())
        return;
    for(auto &c0 : aabb)
        for(auto &c1 : sphere) {
            nngn::vec2 v = {};
            if(!check_bb_sphere_common(
                    c0.pos.xy(), c0.bl, c0.tr, c1.pos.xy(), c1.r, &v))
                continue;
            if(!add_collision(&c0, &c1, {v, 0}, &output->collisions))
                return;
        }
}

void check_bb_sphere(
    std::span<BBCollider> bb, std::span<SphereCollider> sphere,
    Output *output
) {
    { NNGN_STATS_CONTEXT(Colliders, &output->stats.bb_sphere_exec_barrier); }
    NNGN_STATS_CONTEXT(Colliders, &output->stats.bb_sphere_exec);
    if(sphere.empty())
        return;
    for(auto &c0 : bb) {
        const auto pos0 = c0.pos.xy();
        const auto center0 = c0.center.xy();
        for(auto &c1 : sphere) {
            nngn::vec2 v = {};
            if(!check_bb_sphere_common(
                    pos0, c0.bl, c0.tr,
                    center0 + rotate(c1.pos.xy() - center0, c0.cos, -c0.sin),
                    c1.r, &v))
                continue;
            v = rotate(v, c0.cos, c0.sin);
            if(!add_collision(&c0, &c1, {v, 0}, &output->collisions))
                return;
        }
    }
}

void check_sphere_plane(
    std::span<SphereCollider> sphere, std::span<PlaneCollider> plane,
    Output *output
) {
    { NNGN_STATS_CONTEXT(Colliders, &output->stats.sphere_plane_exec_barrier); }
    NNGN_STATS_CONTEXT(Colliders, &output->stats.sphere_plane_exec);
    if(plane.empty())
        return;
    nngn::vec3 v = {};
    for(auto &c0 : sphere)
        for(auto &c1 : plane) {
            const auto n = c1.abcd.xyz();
            const auto d = nngn::Math::dot(n, c0.pos) + c1.abcd[3] - c0.r;
            if(d >= -std::numeric_limits<float>::epsilon())
                continue;
            v = n * -d;
            if(!add_collision(&c0, &c1, v, &output->collisions))
                return;
        }
}

template<typename T> void check_gravity(
    std::span<T> other, std::span<GravityCollider> gravity,
    Output *output
) {
    constexpr auto G = GravityCollider::G;
    { NNGN_STATS_CONTEXT(Colliders, &output->stats.gravity_pos); }
    { NNGN_STATS_CONTEXT(Colliders, &output->stats.gravity_mass); }
    { NNGN_STATS_CONTEXT(Colliders, &output->stats.gravity_max_distance2); }
    { NNGN_STATS_CONTEXT(Colliders, &output->stats.sphere_gravity_exec_barrier); }
    { NNGN_STATS_CONTEXT(Colliders, &output->stats.sphere_gravity_exec); }
    if(gravity.empty())
        return;
    for(auto i0 = begin(other), e0 = end(other); i0 != e0; ++i0) {
        auto &c0 = *i0;
        auto i1 = begin(gravity);
        if constexpr(std::is_same_v<T, GravityCollider>)
            i1 = i0 + 1;
        for(auto e1 = end(gravity); i1 != e1; ++i1) {
            auto &c1 = *i1;
            const auto d = c1.pos - c0.pos;
            const float l2 = nngn::Math::length2(d);
            if(l2 > c1.max_distance2)
                continue;
            const auto v = d * (G * c1.m * c0.m / l2 / std::sqrt(l2));
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

constexpr nngn::vec2 rotate(const nngn::vec2 &p, float cos, float sin) {
    return {p.x * cos - p.y * sin, p.x * sin + p.y * cos};
}

constexpr std::array<nngn::vec2, 4> to_edges(
    const nngn::vec2 &bl, const nngn::vec2 &tr)
{
    return {{{bl.x, bl.y}, {tr.x, bl.y}, {tr.x, tr.y}, {bl.x, tr.y}}};
}

inline bool check_bb_common(
        const nngn::vec2 &bl0, const nngn::vec2 &tr0,
        const std::array<nngn::vec2, 4> &v1, nngn::vec2 *out) {
    const auto [min_x, max_x] = std::minmax_element(
        v1.cbegin(), v1.cend(),
        [](const auto &l, const auto &r) { return l.x < r.x; });
    const float xoverlap = overlap(bl0.x, tr0.x, min_x->x, max_x->x);
    if(float_eq_zero(xoverlap))
        return false;
    const auto [min_y, max_y] = std::minmax_element(
        v1.cbegin(), v1.cend(),
        [](const auto &l, const auto &r) { return l.y < r.y; });
    const float yoverlap = overlap(bl0.y, tr0.y, min_y->y, max_y->y);
    if(float_eq_zero(yoverlap))
        return false;
    *out = std::fabs(xoverlap) <= std::fabs(yoverlap)
        ? nngn::vec2(-xoverlap, 0) : nngn::vec2(0, -yoverlap);
    return true;
}

inline bool check_bb_sphere_common(
        const nngn::vec2 &c0, const nngn::vec2 &bl0, const nngn::vec2 &tr0,
        const nngn::vec2 &sc, float sr, nngn::vec2 *out) {
    const auto nearest = nngn::vec2(
        std::clamp(sc.x, bl0.x, tr0.x),
        std::clamp(sc.y, bl0.y, tr0.y));
    const auto d = sc - nearest;
    if(const auto l2 = nngn::Math::length2(d); l2 != 0) {
        if(l2 >= sr * sr)
            return false;
        *out = d * (1 - sr / std::sqrt(l2));
        return true;
    }
    const auto rd = tr0 - bl0;
    const auto rd_2 = rd / 2.0f;
    const auto v = sc - c0;
    if(v == nngn::vec2())
        return false;
    const auto a = v.y / v.x;
    const auto vx = (v.x > 0 ? 1.0f : -1.0f) * nngn::vec2(rd_2.x, a * rd_2.x);
    const auto vy = (v.y > 0 ? 1.0f : -1.0f) * nngn::vec2(rd_2.y / a, rd_2.y);
    const auto proj = std::abs(vx.y) < std::abs(rd.y) ? vx : vy;
    if(proj == nngn::vec2())
        return false;
    *out = sc - nngn::vec2{c0 + proj * (1 + sr / nngn::Math::length(proj))};
    return true;
}

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
