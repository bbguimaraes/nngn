#include "utils/log.h"

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

void Collider::load(const nngn::lua::table &t) {
    if(const auto x = t.get<std::optional<lua_Integer>>("flags"))
        this->flags = {static_cast<Flag>(*x)};
    this->m = t.get("m", 1.0f);
}

AABBCollider::AABBCollider(vec2 p_bl, vec2 p_tr)
    { set_bb(this, p_bl, p_tr); }

void AABBCollider::update(size_t n, AABBCollider *v) {
    for(auto *e = v + n; v != e;)
        update_bb(v++);
}

void AABBCollider::load(const nngn::lua::table &t) {
    Collider::load(t);
    if(const auto bb_size = t.get<std::optional<float>>("bb")) {
        const float s = *bb_size / 2.0f;
        set_bb(this, {-s, -s}, {s, s});
    } else if(const auto bb = t.get<std::optional<nngn::lua::table>>("bb")) {
        const auto &bb_t = *bb;
        switch(const auto n = bb_t.size()) {
        case 2: {
            const auto w_2 = bb_t.get<float>(1) / 2.0f;
            const auto h_2 = bb_t.get<float>(2) / 2.0f;
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

}
