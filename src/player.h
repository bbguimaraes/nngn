#ifndef NNGN_PLAYER_H
#define NNGN_PLAYER_H

#include <memory>
#include <vector>

struct Entity;

struct Player {
    Entity *e;
    explicit constexpr Player(Entity *p_e) noexcept : e(p_e) {}
};

class Players {
    std::vector<std::unique_ptr<Player>> v = {};
    size_t m_cur = 0;
public:
    size_t n() const { return this->v.size(); }
    size_t idx() const { return this->m_cur; }
    Player *cur() const;
    Player *get(size_t i) const;
    Player *set_idx(size_t i);
    void set_cur(Player *p);
    Player *add(Entity *e);
    void remove(Player *p);
};

#endif
