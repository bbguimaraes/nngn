#include <sol/state_view.hpp>
#include <sol/usertype_proxy.hpp>

#include "entity.h"
#include "luastate.h"

#include "collision/colliders.h"
#include "render/animation.h"
#include "render/light.h"
#include "render/renderers.h"

namespace {

auto to_table(sol::state_view sol, Entities &es, auto &&f) {
    auto ret = sol.create_table();
    std::size_t i = 1;
    for(auto b = es.begin(), e = es.end(); b != e; ++b)
        if(std::invoke(f, *b))
            ret.raw_set(i++, &*b);
    return ret;
}

auto by_name_hash(sol::this_state sol, Entities &e, nngn::Hash h) {
    return to_table(sol, e, [
        h, b = e.begin(), nb = e.name_hashes_begin()
    ](const auto &x) {
        return nb[&x - &*b] == h;
    });
}

auto by_tag_hash(sol::this_state sol, Entities &e, nngn::Hash h) {
    return to_table(sol, e, [
        h, b = e.begin(), tb = e.tag_hashes_begin()
    ](const auto &x) {
        return tb[&x - &*b] == h;
    });
}

auto by_name(sol::this_state sol, Entities &e, std::string_view n) {
    return by_name_hash(sol, e, nngn::hash(n));
}

auto by_tag(sol::this_state sol, Entities &e, std::string_view t) {
    return by_tag_hash(sol, e, nngn::hash(t));
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

}

NNGN_LUA_PROXY(Entity,
    sol::no_constructor,
    "SIZEOF", sol::var(sizeof(Entity)),
    "pos", [](const Entity &e) { return std::tuple(e.p.x, e.p.y, e.p.z); },
    "vel", [](const Entity &e) { return std::tuple(e.v.x, e.v.y, e.v.z); },
    "acc", [](const Entity &e) { return std::tuple(e.a.x, e.a.y, e.a.z); },
    "max_vel", [](const Entity &e) { return e.max_v; },
    "renderer", [](const Entity &e) { return e.renderer; },
    "collider", [](const Entity &e) { return e.collider; },
    "animation", [](const Entity &e) { return e.anim; },
    "light", [](const Entity &e) { return e.light; },
    "parent", [](const Entity &e) { return e.parent; },
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
    "set_camera", &Entity::set_camera,
    "set_light", &Entity::set_light,
    "set_parent", &Entity::set_parent)
NNGN_LUA_PROXY(Entities,
    sol::no_constructor,
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
    "set_tag", &Entities::set_tag)
