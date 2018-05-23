#include <algorithm>
#include <cassert>
#include <cstring>

#include "entity.h"

#include "collision/colliders.h"
#include "math/camera.h"
#include "math/math.h"
#include "render/animation.h"
#include "render/renderers.h"
#include "timing/profile.h"
#include "timing/timing.h"
#include "utils/log.h"
#include "utils/utils.h"

using nngn::vec3;

namespace {

std::ptrdiff_t offset(const Entities &es, const Entity &e) {
    assert(nngn::contains(es, e));
    return std::distance(&*es.cbegin(), &e);
}

decltype(auto) name(Entities *es, Entity *e) {
    return *(es->names_begin() + offset(*es, *e));
}

decltype(auto) tag(Entities *es, Entity *e) {
    return *(es->tags_begin() + offset(*es, *e));
}

decltype(auto) name_hash(Entities *es, Entity *e) {
    return *(es->name_hashes_begin() + offset(*es, *e));
}

decltype(auto) tag_hash(Entities *es, Entity *e) {
    return *(es->tag_hashes_begin() + offset(*es, *e));
}

void copy(std::span<char, 32> v, nngn::Hash *h, std::string_view s) {
    const auto n = std::min(v.size() - 1, s.size());
    std::memcpy(v.data(), s.data(), n);
    v[n] = 0;
    *h = nngn::hash({v.data(), n});
}

void set_pos(Entity *e, vec3 oldp, vec3 p) {
    if(const auto *const parent = e->parent) {
        const auto pp = parent->p;
        oldp += pp, p += pp;
    }
    if(e->renderer)
        e->renderer->set_pos(p);
    if(e->camera)
        e->camera->set_pos(e->camera->p + p - oldp);
    if(e->collider)
        e->collider->pos = p;
}

}

void Entity::set_pos(vec3 pos) {
    ::set_pos(this, this->p, pos);
    this->p = pos;
    this->flags.set(Flag::POS_UPDATED);
}

void Entity::set_vel(vec3 vel) {
    this->v = vel;
    if(this->collider)
        this->collider->vel = v;
}

void Entity::set_renderer(nngn::Renderer *r) {
    if((this->renderer = r)) {
        r->entity = this;
        r->set_pos(this->p);
    }
}

void Entity::set_collider(nngn::Collider *c) {
    if(!(this->collider = c))
        return;
    c->entity = this;
    c->pos = this->p;
    c->vel = this->v;
}

void Entity::set_animation(nngn::Animation *p_a) {
    if((this->anim = p_a))
        p_a->entity = this;
}

void Entity::set_camera(nngn::Camera *c) {
    if((this->camera = c))
        c->set_pos({this->p.xy(), c->p.z});
}

void Entity::set_parent(Entity *e) {
    this->parent = e;
    this->set_pos(this->p);
}

void Entities::set_max(std::size_t n) {
    this->v.set_capacity(n);
    this->names.resize(n);
    this->tags.resize(n);
    this->name_hashes.resize(n);
    this->tag_hashes.resize(n);
}

Entity *Entities::add() {
    if(this->v.full())
        return nngn::Log::l() << "cannot add more entities\n", nullptr;
    auto &ret = this->v.emplace();
    ret.flags.set(Entity::Flag::ALIVE);
    const auto i =
        static_cast<std::size_t>(std::distance(this->v.data(), &ret));
    this->names[i] = {};
    this->tags[i] = {};
    this->name_hashes[i] = {};
    this->tag_hashes[i] = {};
    return &ret;
}

std::span<const char, 32> Entities::name(const Entity &e) const {
    return ::name(const_cast<Entities*>(this), const_cast<Entity*>(&e));
}

std::span<const char, 32> Entities::tag(const Entity &e) const {
    return ::tag(const_cast<Entities*>(this), const_cast<Entity*>(&e));
}

nngn::Hash Entities::name_hash(const Entity &e) const {
    return ::name_hash(const_cast<Entities*>(this), const_cast<Entity*>(&e));
}

nngn::Hash Entities::tag_hash(const Entity &e) const {
    return ::tag_hash(const_cast<Entities*>(this), const_cast<Entity*>(&e));
}

void Entities::set_name(Entity *e, std::string_view s) {
    copy(::name(this, e), &::name_hash(this, e), s);
}

void Entities::set_tag(Entity *e, std::string_view s) {
    copy(::tag(this, e), &::tag_hash(this, e), s);
}

void Entities::update(const nngn::Timing &t) {
    NNGN_PROFILE_CONTEXT(entities);
    const auto dt = t.fdt_s();
    for(auto &x : this->v) {
        if(x.a != vec3{}) {
            x.set_vel(x.v + x.a * dt);
            x.a = {};
        }
        if(x.max_v > 0)
            x.v = nngn::Math::clamp_len(x.v, x.max_v);
        if(x.v != vec3{})
            x.set_pos(x.p + x.v * dt);
    }
}

void Entities::update_children() {
    NNGN_PROFILE_CONTEXT(parents);
    for(auto &x : this->v)
        if(x.parent && x.parent->pos_updated())
            set_pos(&x, x.p, x.p);
}

void Entities::clear_flags() {
    for(auto &x : this->v)
        x.flags.clear(Entity::Flag::POS_UPDATED);
}
