#include <algorithm>

#include "entity.h"

#include "lua/state.h"
#include "timing/profile.h"
#include "timing/timing.h"
#include "utils/log.h"
#include "utils/utils.h"

#include "animation.h"
#include "light.h"
#include "renderers.h"

namespace nngn {

static SpriteAnimation::Group load_group(
    const uvec2 &scale, const nngn::lua::table &t
) {
    const auto read_table = [](const nngn::lua::table &tt, auto &&f) {
        std::vector<decltype(f({}))> v;
        const auto n = tt.size();
        v.reserve(static_cast<std::size_t>(n));
        for(lua_Integer i = 1; i <= n; ++i)
            v.push_back(f(tt[i]));
        return v;
    };
    const auto read_frame = [&scale](const nngn::lua::table &tt) {
        SpriteAnimation::Frame ret = {};
        std::array<uvec2, 2> c = {};
        switch(const auto n = tt.size()) {
        default: Log::l() << "invalid frame length: " << n << std::endl; break;
        case 3:
            c[0] = {tt[1], tt[2]};
            c[1] = c[0] + uvec2(1, 1);
            ret.duration = std::chrono::milliseconds(tt[3]);
            break;
        case 5:
            c[0] = {tt[1], tt[2]};
            c[1] = {tt[3], tt[4]};
            ret.duration = std::chrono::milliseconds(tt[5]);
            break;
        }
        SpriteRenderer::uv_coords(c[0], c[1], scale, ret.uv.data());
        return ret;
    };
    return read_table(t, [read_table, &read_frame](const nngn::lua::table &tt) {
        return read_table(tt, read_frame);
    });
}

void AnimationFunction::load(const nngn::lua::table &t) {
    using D = std::uniform_real_distribution<float>;
    switch(this->type = chain_cast<Type, lua_Integer>(t["type"])) {
    case Type::NONE: break;
    case Type::LINEAR:
        this->linear = {t["v"], t["step_s"], t["end"]};
        break;
    case Type::RANDOM_F:
        this->rnd_f = D(t["min"], t["max"]);
        break;
    case Type::RANDOM_3F: {
        const nngn::lua::table min = t["min"], max = t["max"];
        this->rnd_3f = {
            D(min[1], max[1]),
            D(min[2], max[2]),
            D(min[3], max[3])};
        }
        break;
    }
}

vec3 AnimationFunction::update(const Timing &t, Math::rnd_generator_t *rnd) {
    switch(this->type) {
    case Type::LINEAR:
        this->linear.v += this->linear.step_s * t.fdt_s();
        if(this->linear.step_s > 0)
            this->linear.v = std::min(this->linear.end, this->linear.v);
        else
            this->linear.v = std::max(this->linear.end, this->linear.v);
        return {this->linear.v, 0, 0};
    case Type::RANDOM_F: return {this->rnd_f(*rnd), 0, 0};
    case Type::RANDOM_3F: return {
        this->rnd_3f[0](*rnd),
        this->rnd_3f[1](*rnd),
        this->rnd_3f[2](*rnd)};
    case Type::NONE:
    default: return {};
    };
}

bool AnimationFunction::done() const {
    switch(this->type) {
    case Type::LINEAR: return this->linear.v == this->linear.end;
    case Type::RANDOM_F:
    case Type::RANDOM_3F: return false;
    case Type::NONE:
    default: return true;
    };
}

void SpriteAnimation::load(const nngn::lua::table &t) {
    NNGN_LOG_CONTEXT_CF(SpriteAnimation);
    this->m_group = std::make_shared<Group>(load_group({t[1], t[2]}, t[3]));
}

void SpriteAnimation::load(SpriteAnimation *o) {
    NNGN_LOG_CONTEXT_CF(SpriteAnimation);
    this->m_group = o->m_group;
}

const SpriteAnimation::Frame *SpriteAnimation::cur() const {
    if(!this->m_group)
        return nullptr;
    return &(*this->m_group)[this->m_cur_track][this->cur_frame];
}

void SpriteAnimation::set_track(size_t i) {
    NNGN_LOG_CONTEXT_CF(SpriteAnimation);
    if(const auto n = this->track_count(); i >= n) {
        Log::l() << i << " >= track_count (" << n << ")\n";
        return;
    }
    this->m_cur_track = i;
    this->cur_frame = 0;
    this->timer = std::chrono::microseconds(0);
    this->updated = true;
}

void SpriteAnimation::update(const Timing &t) {
    bool update = std::exchange(this->updated, false);
    const auto *frame = this->cur();
    assert(frame);
    if(frame->duration.count()) {
        const auto dt =
            std::chrono::duration_cast<decltype(this->timer)>(t.dt);
        if((this->timer += dt) >= frame->duration) {
            update = true;
            assert(this->m_group);
            const auto size = (*this->m_group)[this->m_cur_track].size();
            do {
                this->timer -= frame->duration;
                this->cur_frame = (this->cur_frame + 1) % size;
                frame = this->cur();
            } while(this->timer >= frame->duration);
        }
    }
    if(!update)
        return;
    auto *r = static_cast<SpriteRenderer*>(this->entity->renderer);
    const auto &uv = frame->uv;
    r->uv0 = uv[0];
    r->uv1 = uv[1];
    r->flags |= Renderer::Flag::UPDATED;
}

void LightAnimation::load(const nngn::lua::table &t) {
    using D = std::chrono::milliseconds;
    if(const auto o = t.get<std::optional<D::rep>>("rate_ms"))
        this->rate = D{*o};
    if(const auto o = t.get<std::optional<D::rep>>("timer_ms"))
        this->timer = D{*o};
    this->f = {};
    if(const auto o = t.get<std::optional<nngn::lua::table>>("f"))
        this->f.load(*o);
}

void LightAnimation::update(
        const Timing &t, Math::rnd_generator_t *rnd,
        float *a, bool *updated) {
    if(this->f.done())
        return;
    const auto dt = std::chrono::duration_cast<decltype(this->timer)>(t.dt);
    if((this->timer += dt) < this->rate)
        return;
    this->timer -= this->rate;
    *a = this->f.update(t, rnd)[0];
    *updated = true;
}

void LightAnimation::update(const Timing &t, Math::rnd_generator_t *rnd) {
    auto *l = this->entity->light;
    return this->update(t, rnd, &l->color.w, &l->updated);
}

void Animations::set_max(size_t n) {
    set_capacity(&this->sprite, n);
    set_capacity(&this->light, n);
}

void Animations::remove(Animation *p) {
    const auto remove = [p]<typename T>(std::vector<T> *v) {
        const_time_erase(v, static_cast<T*>(p));
        if(p != &*v->end())
            p->entity->anim = p;
    };
    if(contains(this->sprite, *p))
        remove(&this->sprite);
    if(contains(this->light, *p))
        remove(&this->light);
}

Animation *Animations::load(const nngn::lua::table &t) {
    NNGN_LOG_CONTEXT_CF(Animations);
    const auto load = [](const char *n, auto *v, auto &&...args) {
        if(v->size() == v->capacity()) {
            Log::l() << "cannot add more " << n << " animations\n";
            return static_cast<decltype(v->data())>(nullptr);
        }
        auto &ret = v->emplace_back();
        ret.load(args...);
        return &ret;
    };
    if(const auto a = t.get<nngn::lua::sol_user_type<SpriteAnimation*>>("sprite"))
        return load("sprite", &this->sprite, &*a);
    if(const auto o = t.get<std::optional<nngn::lua::table>>("sprite"))
        return load("sprite", &this->sprite, *o);
    if(const auto o = t.get<std::optional<nngn::lua::table>>("light"))
        return load("light", &this->light, *o);
    Log::l() << "no animation data\n";
    return nullptr;
}

void Animations::update(const Timing &t) {
    NNGN_PROFILE_CONTEXT(animations);
    auto *rnd = this->math->rnd_generator();
    for(auto &x : this->sprite)
        x.update(t);
    for(auto &x : this->light)
        x.update(t, rnd);
}

}
