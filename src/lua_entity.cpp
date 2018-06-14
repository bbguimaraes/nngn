#include "lua/function.h"
#include "lua/register.h"
#include "lua/table.h"

#include "entity.h"

#include "math/lua_vector.h"
#include "render/renderers.h"

NNGN_LUA_DECLARE_USER_TYPE(Entity)
NNGN_LUA_DECLARE_USER_TYPE(nngn::Renderer, "Renderer")

namespace {

template<float Entity::*p>
auto get(const Entity &e) {
    return nngn::narrow<lua_Number>(e.*p);
}

template<nngn::vec3 Entity::*p>
auto get(const Entity &e) {
    const auto ret = e.*p;
    return std::tuple{
        nngn::narrow<lua_Number>(ret.x),
        nngn::narrow<lua_Number>(ret.y),
        nngn::narrow<lua_Number>(ret.z),
    };
}

void set_pos(Entity &e, float x, float y, float z) {
    e.set_pos({x, y, z});
}

auto entities_max(const Entities &es) {
    return nngn::narrow<lua_Integer>(es.max());
}

auto entities_n(const Entities &es) {
    return nngn::narrow<lua_Integer>(es.n());
}

auto entities_set_max(Entities &es, lua_Integer n) {
    es.set_max(nngn::narrow<std::size_t>(n));
}

auto filter_to_table(nngn::lua::state_view lua, Entities &es, auto &&f) {
    std::ptrdiff_t i = 0;
    lua_Integer ti = 1;
    auto ret = lua.create_table();
    for(auto &x : es)
        if(std::invoke(f, i++))
            ret.raw_set(ti++, &x);
    return ret;
}

template<typename T>
auto by_name_hash(Entities &e, T th, nngn::lua::state_view lua) {
    const auto h = nngn::narrow<nngn::Hash>(th);
    auto nb = e.name_hashes_begin();
    return filter_to_table(lua, e, [h, nb](auto i) {
        return nb[i] == h;
    });
}

template<typename T>
auto by_tag_hash(Entities &e, T th, nngn::lua::state_view lua) {
    const auto h = nngn::narrow<nngn::Hash>(th);
    auto tb = e.tag_hashes_begin();
    return filter_to_table(lua, e, [h, tb](auto i) {
        return tb[i] == h;
    });
}

auto by_name(Entities &e, std::string_view n, nngn::lua::state_view lua) {
    return by_name_hash(e, nngn::hash(n), lua);
}

auto by_tag(Entities &e, std::string_view t, nngn::lua::state_view lua) {
    return by_tag_hash(e, nngn::hash(t), lua);
}

auto by_idx(Entities &e, lua_Integer i) {
    return &(e.begin()[nngn::narrow<std::ptrdiff_t>(i)]);
}

const char *name(const Entities &es, const Entity &e) {
    return es.name(e).data();
}

const char *tag(const Entities &es, const Entity &e) {
    return es.tag(e).data();
}

template<std::size_t N>
void entities_set_pos(Entities &es, const nngn::lua_vector<std::byte> &v) {
    const auto n = v.size() / sizeof(float);
    assert(n * sizeof(float) == v.size());
    assert(n <= es.n() * N);
    const auto b = es.begin();
    const auto *p = nngn::byte_cast<const float*>(v.data());
    for(std::size_t i = 0; i < n; i += N) {
        auto &x = *(b + static_cast<std::ptrdiff_t>(i / N));
        if constexpr(N == 2)
            x.set_pos({p[i], p[i + 1]});
        else if constexpr(N == 3 || N == 4)
            x.set_pos({p[i], p[i + 1], p[i + 2]});
    }
}

void register_entity(nngn::lua::table_view t) {
    t["SIZEOF"] = nngn::narrow<lua_Integer>(sizeof(Entity));
    t["pos"] = get<&Entity::p>;
    t["renderer"] = [](const Entity &e) { return e.renderer; };
    t["set_pos"] = set_pos;
    t["set_renderer"] = &Entity::set_renderer;
}

void register_entities(nngn::lua::table_view t) {
    t["max"] = entities_max;
    t["n"] = entities_n;
    t["set_max"] = entities_set_max;
    t["add"] = &Entities::add;
    t["idx"] = by_idx;
    t["by_name"] = by_name;
    t["by_name_hash"] = by_name_hash<lua_Integer>;
    t["by_tag"] = by_tag;
    t["by_tag_hash"] = by_tag_hash<lua_Integer>;
    t["name"] = name;
    t["tag"] = tag;
    t["set_name"] = &Entities::set_name;
    t["set_tag"] = &Entities::set_tag;
    t["set_pos2"] = entities_set_pos<2>;
    t["set_pos3"] = entities_set_pos<3>;
    t["set_pos4"] = entities_set_pos<4>;
}

}

NNGN_LUA_DECLARE_USER_TYPE(Entities)
NNGN_LUA_PROXY(Entity, register_entity)
NNGN_LUA_PROXY(Entities, register_entities)
