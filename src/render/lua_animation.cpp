#include <lua.hpp>
#include <ElysianLua/elysian_lua_table_proxy.hpp>
#include <ElysianLua/elysian_lua_thread.hpp>
#include <sol/state_view.hpp>
#include <sol/usertype_proxy.hpp>
#include "../xxx_elysian_lua_push_sol.h"
#include "../xxx_elysian_lua_push_sol_table.h"

#include "luastate.h"

#include "animation.h"

using nngn::AnimationFunction;
using nngn::Animation;
using nngn::Animations;
using nngn::SpriteAnimation;

namespace {

auto load_v(Animations &a, const elysian::lua::StaticStackTable &t) {
    const auto &L = *t.getThread();
    const auto n = t.getLength();
    const elysian::lua::StaticStackTable ret =
        L.createTable(static_cast<int>(n));
    const auto top = L.getTop() + 1;
    for(lua_Integer i = 1; i <= n; ++i) {
        L.push(t[i]);
        const auto ti = L.toValue<elysian::lua::StaticStackTable>(top);
        ret.setFieldRaw(i, sol_usertype_wrapper(a.load(ti)));
        L.pop(1);
    }
    return ret;
}

auto remove_v(Animations &a, const elysian::lua::StaticStackTable &t) {
    for(lua_Integer i = 1, n = t.getLength(); i <= n; ++i)
        a.remove(t[i].get<sol_usertype_wrapper<Animation*>>().get());
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
    "SIZEOF_LIGHT", sol::var(sizeof(nngn::LightAnimation)),
    "sprite", [](Animation *a) { return static_cast<SpriteAnimation*>(a); })
NNGN_LUA_PROXY(Animations,
    sol::no_constructor,
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
