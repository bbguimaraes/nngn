#include "colliders.h"

#include <cmath>

#include "utils/log.h"

namespace {

void update_collider(nngn::Collider *c) {
    c->flags.clear(nngn::Collider::Flag::COLLIDING);
}

void set_bb(nngn::AABBCollider *c, nngn::vec2 bl, nngn::vec2 tr) {
    const auto hs = (tr - bl) / 2.0f;
    c->rel_center = bl + hs;
    c->rel_bl = bl;
    c->rel_tr = tr;
    c->radius = std::sqrt(hs.x * hs.x + hs.y * hs.y);
}

void update_bb(nngn::AABBCollider *c) {
    update_collider(c);
    const auto p = c->pos.xy();
    c->center = p + c->rel_center;
    c->bl = p + c->rel_bl;
    c->tr = p + c->rel_tr;
}

}

namespace nngn {

void Collider::load(nngn::lua::table_view t) {
    if(const auto x = t["flags"].get<std::optional<Flag>>())
        this->flags = {*x};
    this->m = t["m"].get(1.0f);
}

AABBCollider::AABBCollider(vec2 p_bl, vec2 p_tr) {
    set_bb(this, p_bl, p_tr);
}

void AABBCollider::update(std::span<AABBCollider> s) {
    for(auto &x : s)
        update_bb(&x);
}

void AABBCollider::load(nngn::lua::table_view t) {
    Collider::load(t);
    const nngn::lua::value bb = t.state().push(t["bb"]);
    if(const auto type = bb.get_type(); type == nngn::lua::type::number) {
        const float s = bb.get<float>() / 2.0f;
        set_bb(this, {-s, -s}, {s, s});
    } else if(type == nngn::lua::type::table) {
        const auto &bb_t = bb.get<nngn::lua::table_view>();
        switch(const auto n = bb_t.size()) {
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
            Log::l() << "invalid bounding box table size: " << n << '\n';
            break;
        }
    }
}

void BBCollider::update(std::span<BBCollider> s) {
    for(auto &x : s)
        update_bb(&x);
}

void BBCollider::load(nngn::lua::table_view t) {
    AABBCollider::load(t);
    if(const auto r = t["rot"].get<std::optional<float>>()) {
        this->cos = std::cos(*r);
        this->sin = std::sin(*r);
    }
}

}
