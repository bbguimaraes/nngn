#include <sol/table.hpp>

#include "entity.h"
#include "luastate.h"

#include "timing/profile.h"
#include "utils/log.h"
#include "utils/utils.h"

#include "collision.h"

namespace nngn {

Colliders::Colliders()
    { nngn::Stats::reserve(Colliders::STATS_IDX, &this->output.stats); }
Colliders::~Colliders() { nngn::Stats::release(Colliders::STATS_IDX); }

bool Colliders::set_max_colliders(size_t n) {
    this->m_flags.set(Flag::MAX_COLLIDERS_UPDATED);
    this->m_max_colliders = n;
    set_capacity(&this->input.aabb, n);
    return !this->backend || this->backend->set_max_colliders(n);
}

bool Colliders::set_max_collisions(size_t n) {
    this->m_flags.set(Flag::MAX_COLLISIONS_UPDATED);
    set_capacity(&this->output.collisions, n);
    return !this->backend || this->backend->set_max_collisions(n);
}

bool Colliders::set_backend(std::unique_ptr<Backend> p) {
    if(p && !p->init())
       return false;
    this->backend = std::move(p);
    return true;
}

void Colliders::remove(Collider *p) {
    const auto is_in = [p](const auto &v)
        { return v.data() <= p && p < v.data() + v.size(); };
    const auto remove = [p](auto *v) {
        using T = typename std::decay_t<decltype(*v)>::pointer;
        if(auto i = vector_linear_erase(v, static_cast<T>(p)); i != end(*v))
            i->entity->collider = &(*i);
    };
    if(is_in(this->input.aabb))
        remove(&this->input.aabb);
}

void Colliders::clear() {
    this->input.aabb.clear();
}

bool Colliders::check_collisions(const Timing &t) {
    NNGN_LOG_CONTEXT_CF(Colliders);
    NNGN_PROFILE_CONTEXT(collision_check);
    if(!this->m_flags.is_set(Flag::CHECK) || !this->backend)
        return true;
    AABBCollider::update(this->input.aabb.size(), this->input.aabb.data());
    this->output.collisions.clear();
    constexpr auto f =
        Flag::MAX_COLLIDERS_UPDATED | Flag::MAX_COLLISIONS_UPDATED;
    if(this->m_flags.is_set(f)) {
        this->m_flags.clear(f);
        this->backend->set_max_colliders(this->m_max_colliders);
    }
    return this->backend->check(t, this->input, &this->output);
}

void Colliders::resolve_collisions() const {
    NNGN_PROFILE_CONTEXT(collision_resolve);
    if(!this->m_flags.is_set(Flag::RESOLVE))
        return;
    constexpr auto div = [](auto x, auto y)
        { return std::isinf(x) && std::isinf(y) ? 1.0f : x / y; };
    for(const auto &c : this->output.collisions) {
        if(!(c.flags0 & c.flags1 & Collider::Flag::SOLID))
            continue;
        const auto denom = c.mass0 + c.mass1;
        if(const auto f = div(c.mass1, denom); f != 0)
            c.entity0->set_pos(c.entity0->p + f * c.force);
        if(const auto f = div(c.mass0, denom); f != 0)
            c.entity1->set_pos(c.entity1->p - f * c.force);
    }
}

void Colliders::lua_on_collision(lua_State *L) {
    NNGN_PROFILE_CONTEXT(collision_lua);
    if(!this->m_flags.is_set(Flag::RESOLVE) || this->output.collisions.empty())
        return;
    assert(!lua_gettop(L));
    lua_pushcfunction(L, LuaState::msgh);
    if(lua_getglobal(L, "on_collision") != LUA_TFUNCTION) {
        lua_pop(L, 2);
        return;
    }
    for(const auto &x : this->output.collisions) {
        if(~(x.flags0 | x.flags1) & Collider::Flag::TRIGGER)
            continue;
        lua_pushvalue(L, 2);
        sol::stack::push(L, x.entity0);
        sol::stack::push(L, x.entity1);
        lua_createtable(L, 3, 0);
        lua_pushnumber(L, static_cast<lua_Number>(x.force[0]));
        lua_rawseti(L, -2, 1);
        lua_pushnumber(L, static_cast<lua_Number>(x.force[1]));
        lua_rawseti(L, -2, 2);
        lua_pushnumber(L, static_cast<lua_Number>(x.force[2]));
        lua_rawseti(L, -2, 3);
        if(lua_pcall(L, 3, 0, 1) != LUA_OK)
            lua_pop(L, 1);
    }
    lua_pop(L, 2);
}

template<typename T>
static typename T::pointer add(T *v, typename T::const_reference c) {
    if(v->size() < v->capacity())
        return &v->emplace_back(c);
    Log::l() << "cannot add more" << std::endl;
    return nullptr;
}

AABBCollider *Colliders::add(const AABBCollider &c)
    { NNGN_LOG_CONTEXT("aabb"); return nngn::add(&this->input.aabb, c); }

Collider *Colliders::load(const sol::stack_table &t) {
    NNGN_LOG_CONTEXT_CF(Colliders);
    const auto load = [this, &t](auto c) { c.load(t); return this->add(c); };
    switch(const Collider::Type type = t["type"]) {
    case Collider::Type::AABB: return load(AABBCollider());
    case Collider::Type::NONE:
    case Collider::Type::N_TYPES:
    default:
        Log::l() << "invalid type: " << static_cast<int>(type) << '\n';
        return nullptr;
    }
}

}
