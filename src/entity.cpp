#include <algorithm>
#include <cassert>
#include <cstring>

#include "entity.h"

#include "render/renderers.h"
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

void set_pos(Entity *e, vec3 p) {
    if(e->renderer)
        e->renderer->set_pos(p);
}

}

void Entity::set_pos(vec3 pos) {
    ::set_pos(this, pos);
    this->p = pos;
    this->flags |= Flag::POS_UPDATED;
}

void Entity::set_renderer(nngn::Renderer *r) {
    if((this->renderer = r)) {
        r->entity = this;
        r->set_pos(this->p);
    }
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

void Entities::clear_flags() {
    for(auto &x : this->v)
        x.flags.clear(Entity::Flag::POS_UPDATED);
}
