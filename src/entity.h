#ifndef NNGN_ENTITY_H
#define NNGN_ENTITY_H

#include <array>
#include <memory>
#include <string_view>
#include <vector>

#include "math/hash.h"
#include "math/vec3.h"
#include "utils/flags.h"
#include "utils/static_vector.h"
#include "utils/utils.h"

namespace nngn {
    struct Renderer;
}

struct Entity {
    enum Flag : std::uintptr_t {
        ALIVE = 1u << 0, POS_UPDATED = 1u << 1,
    };
    nngn::vec3 p = {0, 0, 0};
    nngn::Flags<Flag> flags = {};
    nngn::Renderer *renderer = nullptr;
    bool pos_updated() const { return this->flags.is_set(Flag::POS_UPDATED); }
    void set_pos(nngn::vec3 p);
    void set_renderer(nngn::Renderer *p);
};

class Entities {
    nngn::static_vector<Entity> v = {};
    std::vector<std::array<char, 32>> names = {}, tags = {};
    std::vector<nngn::Hash> name_hashes = {}, tag_hashes = {};
public:
    size_t max() const { return this->v.capacity(); }
    size_t n() const { return this->v.size(); }
    void set_max(std::size_t n);
    Entity *add();
    void remove(Entity *e) { this->v.erase(e); }
    NNGN_EXPOSE_ITERATOR(, v)
    NNGN_EXPOSE_ITERATOR(names_, names)
    NNGN_EXPOSE_ITERATOR(tags_, tags)
    NNGN_EXPOSE_ITERATOR(name_hashes_, name_hashes)
    NNGN_EXPOSE_ITERATOR(tag_hashes_, tag_hashes)
    std::span<const char, 32> name(const Entity &e) const;
    std::span<const char, 32> tag(const Entity &e) const;
    nngn::Hash name_hash(const Entity &e) const;
    nngn::Hash tag_hash(const Entity &e) const;
    void set_name(Entity *e, std::string_view s);
    void set_tag(Entity *e, std::string_view s);
    void clear_flags();
};

#endif
