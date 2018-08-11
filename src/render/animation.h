#ifndef NNGN_ANIMATION_H
#define NNGN_ANIMATION_H

#include <array>
#include <chrono>
#include <memory>
#include <vector>

#include "lua/table.h"
#include "math/math.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include "utils/flags.h"

struct lua_State;
struct Entity;

namespace nngn {

struct SpriteRenderer;
struct Timing;

class AnimationFunction {
public:
    enum class Type : uint8_t {
        NONE, LINEAR, RANDOM_F, RANDOM_3F,
    };
private:
    Type type = {};
    struct linear_t { float v = 0, step_s = 0, end = 0; };
    union {
        linear_t linear = {};
        std::uniform_real_distribution<float> rnd_f;
        std::array<std::uniform_real_distribution<float>, 3> rnd_3f;
    };
public:
    AnimationFunction() noexcept : linear() {}
    void load(const nngn::lua::table &t);
    vec3 update(const Timing &t, Math::rnd_generator_t *rnd);
    bool done() const;
};

struct Animation {
    Entity *entity = nullptr;
};

class SpriteAnimation : public Animation {
public:
    struct Frame {
        std::array<vec2, 2> uv;
        std::chrono::milliseconds duration;
    };
    using Track = std::vector<Frame>;
    using Group = std::vector<Track>;
private:
    std::shared_ptr<const Group> m_group = {};
    size_t m_cur_track = 0, cur_frame = 0;
    std::chrono::microseconds timer = {};
    bool updated = true;
public:
    void load(const nngn::lua::table &t);
    void load(SpriteAnimation *o);
    auto cur_track() const { return this->m_cur_track; }
    auto track_count() const { return this->m_group->size(); }
    const Frame *cur() const;
    void set_track(size_t index);
    void update(const Timing &t);
};

class Animations {
    Math *math = nullptr;
    std::vector<SpriteAnimation> sprite = {};
public:
    void init(Math *m) { this->math = m; }
    size_t max() const { return this->max_sprite(); }
    size_t max_sprite() const { return this->sprite.capacity(); }
    size_t n() const { return this->n_sprite(); }
    size_t n_sprite() const { return this->sprite.size(); }
    void set_max(size_t n);
    void remove(Animation *p);
    Animation *load(const nngn::lua::table &t);
    void update(const Timing &t);
};

}

#endif
