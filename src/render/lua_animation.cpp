#include <sol/state_view.hpp>
#include <sol/usertype_proxy.hpp>

#include "luastate.h"

#include "animation.h"

using nngn::AnimationFunction;
using nngn::Animation;
using nngn::Animations;
using nngn::SpriteAnimation;

namespace {

auto load_v(sol::this_state sol, Animations &a, const sol::stack_table &t) {
    auto ret = sol::stack_table(sol, sol::new_table(1));
    for(size_t i = 1, n = t.size(); i <= n; ++i)
        ret.raw_set(i, a.load(t[i]));
    return ret;
}

auto remove_v(Animations &a, const sol::stack_table &t) {
    for(size_t i = 1, n = t.size(); i <= n; ++i)
        a.remove(t[i]);
}

}

NNGN_LUA_PROXY(AnimationFunction,
    sol::no_constructor,
    "LINEAR", sol::var(AnimationFunction::Type::LINEAR),
    "RANDOM_F", sol::var(AnimationFunction::Type::RANDOM_F),
    "RANDOM_3F", sol::var(AnimationFunction::Type::RANDOM_3F))
NNGN_LUA_PROXY(SpriteAnimation,
    sol::no_constructor,
    "cur_track", &SpriteAnimation::cur_track,
    "track_count", &SpriteAnimation::track_count,
    "set_track", &SpriteAnimation::set_track)
NNGN_LUA_PROXY(Animation,
    sol::no_constructor,
    "SIZEOF_SPRITE", sol::var(sizeof(nngn::SpriteAnimation)),
    "sprite", [](Animation *a) { return static_cast<SpriteAnimation*>(a); })
NNGN_LUA_PROXY(Animations,
    sol::no_constructor,
    "max", &Animations::max,
    "max_sprite", &Animations::max_sprite,
    "n", &Animations::n,
    "n_sprite", &Animations::n_sprite,
    "set_max", &Animations::set_max,
    "load", &Animations::load,
    "load_v", load_v,
    "remove", &Animations::remove,
    "remove_v", remove_v)
