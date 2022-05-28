#include "lua/state.h"

#include "animation.h"

using nngn::AnimationFunction;
using nngn::Animation;
using nngn::Animations;
using nngn::SpriteAnimation;

namespace {

auto load_v(
    Animations &a, nngn::lua::table_view t, nngn::lua::state_arg lua
) {
    auto ret = nngn::lua::state_view{lua}.create_table(1, 0);
    for(lua_Integer i = 1, n = t.size(); i <= n; ++i)
        ret.raw_set(i, nngn::lua::sol_user_type{a.load(t[i])});
    return ret.release();
}

auto remove_v(Animations &a, const nngn::lua::table &t) {
    for(lua_Integer i = 1, n = t.size(); i <= n; ++i)
        a.remove(static_cast<nngn::lua::sol_user_type<Animation*>>(t[i]));
}

}

NNGN_LUA_PROXY(AnimationFunction,
    "LINEAR", nngn::lua::var(AnimationFunction::Type::LINEAR),
    "RANDOM_F", nngn::lua::var(AnimationFunction::Type::RANDOM_F),
    "RANDOM_3F", nngn::lua::var(AnimationFunction::Type::RANDOM_3F))
NNGN_LUA_PROXY(SpriteAnimation,
    "cur_track", &SpriteAnimation::cur_track,
    "track_count", &SpriteAnimation::track_count,
    "set_track", &SpriteAnimation::set_track)
NNGN_LUA_PROXY(Animation,
    "SIZEOF_SPRITE", nngn::lua::var(sizeof(nngn::SpriteAnimation)),
    "SIZEOF_LIGHT", nngn::lua::var(sizeof(nngn::LightAnimation)),
    "sprite", [](Animation *a) { return static_cast<SpriteAnimation*>(a); })
NNGN_LUA_PROXY(Animations,
    "max", &Animations::max,
    "max_sprite", &Animations::max_sprite,
    "max_light", &Animations::max_light,
    "n", &Animations::n,
    "n_sprite", &Animations::n_sprite,
    "n_light", &Animations::n_light,
    "set_max", &Animations::set_max,
    "load", &Animations::load,
    "load_v", load_v,
    "remove", &Animations::remove,
    "remove_v", remove_v)
