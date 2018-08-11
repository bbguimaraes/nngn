#include <algorithm>

#include "entity.h"

#include "lua/iter.h"
#include "lua/register.h"
#include "timing/profile.h"
#include "timing/timing.h"
#include "utils/log.h"
#include "utils/utils.h"

#include "animation.h"
#include "renderers.h"

using nngn::uvec2;

NNGN_LUA_DECLARE_USER_TYPE(nngn::SpriteAnimation, "SpriteAnimation")

namespace {

auto load_frame(uvec2 scale, nngn::lua::table_view t) {
    NNGN_LOG_CONTEXT_F();
    NNGN_ANON_DECL(nngn::lua::stack_mark(t.state()));
    nngn::SpriteAnimation::Frame ret = {};
    uvec2 c0 = {}, c1 = {};
    switch(const auto n = t.size()) {
    case 3:
        c0 = {t[1], t[2]};
        c1 = c0 + uvec2{1};
        ret.duration = std::chrono::milliseconds{t[3]};
        break;
    case 5:
        c0 = {t[1], t[2]};
        c1 = {t[3], t[4]};
        ret.duration = std::chrono::milliseconds{t[5]};
        break;
    default:
        NNGN_LOG_CONTEXT_F();
        nngn::Log::l() << "invalid frame length: " << n << '\n';
        break;
    }
    nngn::SpriteRenderer::uv_coords(
        c0, c1, scale, nngn::SpriteRenderer::uv_span(&ret.uv));
    return ret;
}

auto load_track(uvec2 scale, nngn::lua::table_view t) {
    NNGN_LOG_CONTEXT_F();
    NNGN_ANON_DECL(nngn::lua::stack_mark(t.state()));
    std::vector<nngn::SpriteAnimation::Frame> ret = {};
    ret.reserve(nngn::narrow<std::size_t>(t.size()));
    for(auto [_, frame] : ipairs(t))
        ret.push_back(load_frame(scale, frame));
    return ret;
}

auto load_group(uvec2 scale, nngn::lua::table_view t) {
    NNGN_LOG_CONTEXT_F();
    NNGN_ANON_DECL(nngn::lua::stack_mark(t.state()));
    std::vector<std::vector<nngn::SpriteAnimation::Frame>> ret = {};
    ret.reserve(nngn::narrow<std::size_t>(t.size()));
    for(auto [_, track] : ipairs(t))
        ret.emplace_back(load_track(scale, track));
    return nngn::SpriteAnimation::Group{ret};
}

}

namespace nngn {

void AnimationFunction::load(nngn::lua::table_view t) {
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

void SpriteAnimation::load(nngn::lua::table_view t) {
    NNGN_LOG_CONTEXT_CF(SpriteAnimation);
    NNGN_ANON_DECL(nngn::lua::stack_mark(t.state()));
    this->m_group = std::make_shared<Group>(
        load_group(uvec2{t[1], t[2]}, nngn::lua::table{t[3]}));
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
    r->uv = frame->uv;
    r->flags |= Renderer::Flag::UPDATED;
}

void Animations::set_max(size_t n) {
    set_capacity(&this->sprite, n);
}

void Animations::remove(Animation *p) {
    const auto remove = [p]<typename T>(std::vector<T> *v) {
        const_time_erase(v, static_cast<T*>(p));
        if(p != &*v->end())
            p->entity->anim = p;
    };
    if(contains(this->sprite, *p))
        remove(&this->sprite);
}

Animation *Animations::load(nngn::lua::table_view t) {
    NNGN_LOG_CONTEXT_CF(Animations);
    const auto lua = t.state();
    NNGN_ANON_DECL(nngn::lua::stack_mark(lua));
    const auto load = [](const char *n, auto *v, auto &&...args) {
        if(v->size() == v->capacity()) {
            Log::l() << "cannot add more " << n << " animations\n";
            return static_cast<decltype(v->data())>(nullptr);
        }
        auto &ret = v->emplace_back();
        ret.load(args...);
        return &ret;
    };
    const nngn::lua::value v = lua.push(t["sprite"]);
    if(const auto type = v.get_type(); type == nngn::lua::type::table)
        return load("sprite", &this->sprite, v.get<nngn::lua::table_view>());
    else if(type == nngn::lua::type::user_data)
        return load("sprite", &this->sprite, v.get<SpriteAnimation*>());
    Log::l() << "no animation data\n";
    return nullptr;
}

void Animations::update(const Timing &t) {
    NNGN_PROFILE_CONTEXT(animations);
    for(auto &x : this->sprite)
        x.update(t);
}

}
