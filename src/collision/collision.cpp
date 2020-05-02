#include "entity.h"

#include <cmath>

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
    set_capacity(&this->input.bb, n);
    set_capacity(&this->input.sphere, n);
    set_capacity(&this->input.plane, n);
    set_capacity(&this->input.gravity, n);
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
    if(contains(this->input.bb, *p))
        remove(&this->input.bb);
    if(contains(this->input.sphere, *p))
        remove(&this->input.sphere);
    if(contains(this->input.plane, *p))
        remove(&this->input.plane);
    if(contains(this->input.gravity, *p))
        remove(&this->input.gravity);
}

void Colliders::clear() {
    this->input.aabb.clear();
    this->input.bb.clear();
    this->input.sphere.clear();
    this->input.plane.clear();
    this->input.gravity.clear();
}

bool Colliders::check_collisions(const Timing &t) {
    NNGN_LOG_CONTEXT_CF(Colliders);
    NNGN_PROFILE_CONTEXT(collision_check);
    AABBCollider::update(this->input.aabb);
    BBCollider::update(this->input.bb);
    SphereCollider::update(this->input.sphere);
    PlaneCollider::update(this->input.plane);
    GravityCollider::update(this->input.gravity);
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

void Colliders::resolve_collisions(void) const {
    NNGN_PROFILE_CONTEXT(collision_resolve);
    if(!this->m_flags.is_set(Flag::RESOLVE))
        return;
//    const auto dt = t.fdt_s(), dt2 = dt * dt * 32;
    constexpr auto div = [](auto x, auto y)
        { return std::isinf(x) && std::isinf(y) ? 1.0f : x / y; };
    for(const auto &c : this->output.collisions) {
        if(!(c.flags0 & c.flags1 & Collider::Flag::SOLID))
            continue;
        const auto denom = c.mass0 + c.mass1;
        if(const auto f = 2.0f * div(c.mass1, denom); f != 0) {
            c.entity0->set_pos(c.entity0->p + c.normal * c.length * f/* * .5f*/);
//            if(c.e0->v != glm::vec3(0))
//                c.e0->set_vel(c.e0->v - c.n * glm::dot(c.e0->v, c.n));
        }
        if(!c.entity1)
            continue;
        if(const auto f = 2.0f * div(c.mass0, denom); f != 0) {
            c.entity1->set_pos(c.entity1->p - c.normal * c.length * f/* * .5f*/);
//            if(c.e1->v != glm::vec3(0))
//                c.e1->set_vel(c.e1->v - c.n * glm::dot(c.e1->v, c.n));
        }
    }
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
    const auto t = lua.create_table(4, 0).release();
    pop.set_n(3);
    return std::all_of(
        begin(this->output.collisions), end(this->output.collisions),
        [&lua, &msgh, &f, &t](const auto &x) {
            if(~(x.flags0 | x.flags1) & Collider::Flag::TRIGGER)
                return true;
            t.raw_set(1, nngn::narrow<lua_Number>(x.normal));
            t.raw_set(2, nngn::narrow<lua_Number>(x.force[0]));
            t.raw_set(3, nngn::narrow<lua_Number>(x.force[1]));
            t.raw_set(4, nngn::narrow<lua_Number>(x.force[2]));
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
BBCollider *Colliders::add(const BBCollider &c)
    { NNGN_LOG_CONTEXT("bb"); return nngn::add(&this->input.bb, c); }
SphereCollider *Colliders::add(const SphereCollider &c)
    { NNGN_LOG_CONTEXT("sphere"); return nngn::add(&this->input.sphere, c); }
PlaneCollider *Colliders::add(const PlaneCollider &c)
    { NNGN_LOG_CONTEXT("plane"); return nngn::add(&this->input.plane, c); }
GravityCollider *Colliders::add(const GravityCollider &c)
    { NNGN_LOG_CONTEXT("gravity"); return nngn::add(&this->input.gravity, c); }

Collider *Colliders::load(nngn::lua::table_view t) {
    NNGN_LOG_CONTEXT_CF(Colliders);
    const auto load = [this, &t](auto c) { c.load(t); return this->add(c); };
    const auto type = chain_cast<Collider::Type, lua_Integer>(t["type"]);
    switch(type) {
    case Collider::Type::AABB: return load(AABBCollider());
    case Collider::Type::BB: return load(BBCollider());
    case Collider::Type::SPHERE: return load(SphereCollider());
    case Collider::Type::PLANE: return load(PlaneCollider());
    case Collider::Type::GRAVITY: return load(GravityCollider());
    case Collider::Type::NONE:
    case Collider::Type::N_TYPES:
    default:
        Log::l() << "invalid type: " << static_cast<int>(type) << '\n';
        return nullptr;
    }
}

}
