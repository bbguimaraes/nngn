#include <sol/state_view.hpp>
#include <sol/usertype_proxy.hpp>

#include "entity.h"
#include "luastate.h"

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
    "renderer", [](const Entity &e) { return e.renderer; },
    "set_pos", [](Entity &e, float v0, float v1, float v2)
        { e.set_pos({v0, v1, v2}); },
    "set_renderer", &Entity::set_renderer)
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
