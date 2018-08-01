#include "entity.h"

#include "lua/state.h"

#include "collision/colliders.h"
#include "render/animation.h"
#include "render/renderers.h"

namespace {

auto to_table(Entities &es, auto &&f, nngn::lua::state_arg lua) {
    auto ret = nngn::lua::state_view{lua}.create_table();
    std::size_t i = 1;
    for(auto &x : es)
        if(std::invoke(f, x))
            ret.raw_set(i++, &x);
    return ret;
}

auto by_name_hash(Entities &e, nngn::Hash h, nngn::lua::state_arg lua) {
    return to_table(e, [
        h, b = e.begin(), nb = e.name_hashes_begin()
    ](const auto &x) {
        return nb[&x - &*b] == h;
    }, lua);
}

auto by_tag_hash(Entities &e, nngn::Hash h, nngn::lua::state_arg lua) {
    return to_table(e, [
        h, b = e.begin(), tb = e.tag_hashes_begin()
    ](const auto &x) {
        return tb[&x - &*b] == h;
    }, lua);
}

auto by_name(Entities &e, std::string_view n, nngn::lua::state_arg lua) {
    return by_name_hash(e, nngn::hash(n), lua);
}

auto by_tag(Entities &e, std::string_view t, nngn::lua::state_arg lua) {
    return by_tag_hash(e, nngn::hash(t), lua);
}

auto by_idx(Entities &e, std::size_t i) {
    return e.begin()[static_cast<std::ptrdiff_t>(i)];
}

const char *name(const Entities &es, const Entity &e) {
    return es.name(e).data();
}

const char *tag(const Entities &es, const Entity &e) {
    return es.tag(e).data();
}

template<std::size_t N>
void set_pos(Entities &es, const std::vector<std::byte> &v) {
    const auto n = v.size() / sizeof(float);
    assert(n * sizeof(float) == v.size());
    assert(n <= es.n() * N);
    const auto b = es.begin();
    const auto *p =
        static_cast<const float*>(static_cast<const void*>(v.data()));
    for(std::size_t i = 0; i < n; i += N)
        if constexpr(N == 2)
            (b + static_cast<std::ptrdiff_t>(i / N))
                ->set_pos({p[i], p[i + 1]});
        else if constexpr(N == 3 || N == 4)
            (b + static_cast<std::ptrdiff_t>(i / N))
                ->set_pos({p[i], p[i + 1], p[i + 2]});
}

}

NNGN_LUA_PROXY(Entity,
    "SIZEOF", nngn::lua::var(sizeof(Entity)),
    "pos", [](const Entity &e) { return std::tuple(e.p.x, e.p.y, e.p.z); },
    "vel", [](const Entity &e) { return std::tuple(e.v.x, e.v.y, e.v.z); },
    "acc", [](const Entity &e) { return std::tuple(e.a.x, e.a.y, e.a.z); },
    "max_vel", [](const Entity &e) { return e.max_v; },
    "renderer", [](const Entity &e) { return e.renderer; },
    "collider", [](const Entity &e) { return e.collider; },
    "animation", [](const Entity &e) { return e.anim; },
    "set_pos", [](Entity &e, float v0, float v1, float v2)
        { e.set_pos({v0, v1, v2}); },
    "set_vel", [](Entity &e, float v0, float v1, float v2)
        { e.set_vel({v0, v1, v2}); },
    "set_acc", [](Entity &e, float v0, float v1, float v2)
        { e.a = {v0, v1, v2}; },
    "set_max_vel", [](Entity &e, float v) { e.max_v = v; },
    "set_renderer", &Entity::set_renderer,
    "set_collider", &Entity::set_collider,
    "set_animation", &Entity::set_animation,
    "set_camera", &Entity::set_camera)
NNGN_LUA_PROXY(Entities,
    "max", &Entities::max,
    "n", &Entities::n,
    "set_max", &Entities::set_max,
    "add", &Entities::add,
    "idx", by_idx,
    "by_name", by_name,
    "by_name_hash", by_name_hash,
    "by_tag", by_tag,
    "by_tag_hash", by_tag_hash,
    "name", name,
    "tag", tag,
    "set_name", &Entities::set_name,
    "set_tag", &Entities::set_tag,
    "set_pos2", set_pos<2>,
    "set_pos3", set_pos<3>,
    "set_pos4", set_pos<4>)
