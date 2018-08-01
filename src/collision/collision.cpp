#include "entity.h"

#include "lua/register.h"
#include "lua/state.h"
#include "lua/utils.h"
#include "timing/profile.h"
#include "utils/log.h"
#include "utils/utils.h"

#include "collision.h"

NNGN_LUA_DECLARE_USER_TYPE(Entity)

namespace nngn {

Colliders::Colliders() {
    nngn::Stats::reserve(Colliders::STATS_IDX, &this->output.stats);
}

Colliders::~Colliders() {
    nngn::Stats::release(Colliders::STATS_IDX);
}

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
    const auto remove = [p]<typename T>(std::vector<T> *v) {
        const_time_erase(v, static_cast<T*>(p));
        if(p != &*v->end())
            p->entity->collider = p;
    };
    if(contains(this->input.aabb, *p))
        remove(&this->input.aabb);
}

void Colliders::clear() {
    this->input.aabb.clear();
}

bool Colliders::check_collisions(const Timing &t) {
    NNGN_LOG_CONTEXT_CF(Colliders);
    NNGN_PROFILE_CONTEXT(collision_check);
    AABBCollider::update(this->input.aabb);
    this->output.collisions.clear();
    if(!this->m_flags.is_set(Flag::CHECK) || !this->backend)
        return true;
    constexpr auto f =
        Flag::MAX_COLLIDERS_UPDATED | Flag::MAX_COLLISIONS_UPDATED;
    if(this->m_flags.is_set(f)) {
        this->m_flags.clear(f);
        this->backend->set_max_colliders(this->m_max_colliders);
    }
    return this->backend->check(t, &this->input, &this->output);
}

bool Colliders::lua_on_collision(nngn::lua::state_view lua) {
    NNGN_PROFILE_CONTEXT(collision_lua);
    if(!this->m_flags.is_set(Flag::RESOLVE) || this->output.collisions.empty())
        return true;
    NNGN_ANON_DECL(nngn::lua::stack_mark(lua));
    const auto msgh = lua.push(nngn::lua::msgh);
    const auto f = lua.push(lua.globals()["on_collision"]);
    auto pop = nngn::lua::defer_pop(lua, 2);
    if(lua.get_type(-1) != nngn::lua::type::function)
        return true;
    const auto t = lua.create_table(3, 0).release();
    pop.set_n(3);
    return std::all_of(
        begin(this->output.collisions), end(this->output.collisions),
        [&lua, &msgh, &f, &t](const auto &x) {
            if(~(x.flags0 | x.flags1) & Collider::Flag::TRIGGER)
                return true;
            t.raw_set(1, nngn::narrow<lua_Number>(x.force[0]));
            t.raw_set(2, nngn::narrow<lua_Number>(x.force[1]));
            t.raw_set(3, nngn::narrow<lua_Number>(x.force[2]));
            return lua.pcall(msgh, f, x.entity0, x.entity1, t) == LUA_OK;
        });
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

Collider *Colliders::load(nngn::lua::table_view t) {
    NNGN_LOG_CONTEXT_CF(Colliders);
    const auto load = [this, &t](auto c) { c.load(t); return this->add(c); };
    const auto type = chain_cast<Collider::Type, lua_Integer>(t["type"]);
    switch(type) {
    case Collider::Type::AABB: return load(AABBCollider());
    case Collider::Type::NONE:
    case Collider::Type::N_TYPES:
    default:
        Log::l() << "invalid type: " << static_cast<int>(type) << '\n';
        return nullptr;
    }
}

}