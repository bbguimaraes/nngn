#include <lua.hpp>
#include <ElysianLua/elysian_lua_function.hpp>
#include <ElysianLua/elysian_lua_table.hpp>
#include <ElysianLua/elysian_lua_table_proxy.hpp>
#include "../xxx_elysian_lua_push_int.h"

#include "utils/log.h"

#include "math/math.h"

#include "colliders.h"

namespace {

void set_bb(nngn::AABBCollider *c, nngn::vec2 bl, nngn::vec2 tr) {
    const auto hs = (tr - bl) / 2.0f;
    c->rel_center = bl + hs;
    c->rel_bl = bl;
    c->rel_tr = tr;
    c->radius = std::sqrt(hs.x * hs.x + hs.y * hs.y);
}

inline void update_bb(nngn::AABBCollider *c) {
    const auto p = c->pos.xy();
    c->center = p + c->rel_center;
    c->bl = p + c->rel_bl;
    c->tr = p + c->rel_tr;
}

}

namespace nngn {

void Collider::load(const elysian::lua::StaticStackTable &t) {
    if(const auto x = t["flags"].get<std::optional<Flag>>())
        this->flags = {*x};
    this->m = t["m"].get<std::optional<float>>().value_or(1);
}

AABBCollider::AABBCollider(vec2 p_bl, vec2 p_tr)
    { set_bb(this, p_bl, p_tr); }

void AABBCollider::update(size_t n, AABBCollider *v) {
    for(auto *e = v + n; v != e;)
        update_bb(v++);
}

void AABBCollider::load(const elysian::lua::StaticStackTable &t) {
    Collider::load(t);
    if(const auto bb_size = t["bb"].get<std::optional<float>>()) {
        const float s = *bb_size / 2.0f;
        set_bb(this, {-s, -s}, {s, s});
    } else if(const auto bb =
            t["bb"].get<std::optional<elysian::lua::Table>>()) {
        const auto bb_t = *bb;
        switch(const auto n = bb_t.getLength()) {
        case 2: {
            const auto w_2 = bb_t[1].get<float>() / 2.0f;
            const auto h_2 = bb_t[2].get<float>() / 2.0f;
            set_bb(this, {-w_2, -h_2}, {w_2, h_2});
            break;
        }
        case 4:
            set_bb(this, {bb_t[1], bb_t[2]}, {bb_t[3], bb_t[4]});
            break;
        default:
            Log::l() << "invalid BB table size: " << n << '\n';
            break;
        }
    }
}

void BBCollider::update(size_t n, BBCollider *v) {
    for(auto *e = v + n; v != e;)
        update_bb(v++);
}

void BBCollider::load(const elysian::lua::StaticStackTable &t) {
    AABBCollider::load(t);
    if(const auto o = t["rot"].get<std::optional<float>>()) {
        const auto r = *o;
        this->cos = std::cos(r);
        this->sin = std::sin(r);
    }
}

void SphereCollider::load(const elysian::lua::StaticStackTable &t) {
    Collider::load(t);
    if(const auto x = t["r"].get<std::optional<float>>())
        this->r = *x;
}

void PlaneCollider::update(size_t n, PlaneCollider *v) {
    for(auto *e = v + n; v != e; ++v)
        v->abcd[3] = -nngn::Math::dot(v->abcd.xyz(), v->pos);
}

void PlaneCollider::load(const elysian::lua::StaticStackTable &t) {
    Collider::load(t);
    if(const auto tt = t["n"].get<std::optional<elysian::lua::Table>>()) {
        const auto tv = *tt;
        this->abcd[0] = tv[1];
        this->abcd[1] = tv[2];
        this->abcd[2] = tv[3];
    }
}

void GravityCollider::load(const elysian::lua::StaticStackTable &t) {
    Collider::load(t);
    const float d = t["max_distance"];
    this->max_distance2 = d * d;
}

}
