#include <algorithm>
#include <cassert>

#include "entity.h"
#include "player.h"

std::tuple<float, float> Player::face_vec(float s) const {
    switch(this->face) {
    case Face::FLEFT: return {-s, 0};
    case Face::FRIGHT: return {s, 0};
    case Face::FDOWN: return {0, -s};
    case Face::FUP: return {0, s};
    case Face::N_FACES:
    default: return {};
    }
}

Player *Players::cur() const {
    return this->v.empty()
        ? nullptr
        : this->v[this->m_cur].get();
}

Player *Players::get(size_t i) const {
    assert(i < this->v.size());
    return this->v[i].get();
}

Player *Players::set_idx(size_t i) {
    assert(i < this->v.size());
    return this->v[this->m_cur = i].get();
}

void Players::set_cur(Player *p) {
    const auto b = this->v.cbegin(), e = this->v.cend();
    const auto it = std::find_if(
        b, e, [p](const auto &x) { return x.get() == p; });
    assert(it != e);
    this->set_idx(static_cast<size_t>(it - b));
}

Player *Players::add(Entity *e) {
    if(this->v.empty())
        this->m_cur = 0;
    return this->v.emplace_back(std::make_unique<Player>(e)).get();
}

void Players::remove(Player *p) {
    const auto b = this->v.cbegin(), e = this->v.cend();
    const auto it = std::find_if(
        b, e, [p](const auto &x) { return x.get() == p; });
    const auto i = static_cast<size_t>(it - b);
    if(i < this->m_cur || (i == this->m_cur && i == this->v.size() - 1))
        --this->m_cur;
    this->v.erase(it);
}
