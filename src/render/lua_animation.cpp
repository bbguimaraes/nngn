#include "lua/function.h"
#include "lua/iter.h"
#include "lua/register.h"
#include "lua/table.h"

#include "animation.h"

using nngn::AnimationFunction;
using nngn::Animation;
using nngn::Animations;
using nngn::SpriteAnimation;

NNGN_LUA_DECLARE_USER_TYPE(Animation)
NNGN_LUA_DECLARE_USER_TYPE(SpriteAnimation)

namespace {

template<nngn::member_function_pointer auto f>
auto get(const nngn::member_obj_type_t<decltype(f)> &t) {
    return nngn::narrow<lua_Integer>((t.*f)());
}

template<nngn::member_function_pointer auto f>
void set(nngn::member_obj_type_t<decltype(f)> &t, lua_Integer n) {
    (t.*f)(nngn::narrow<std::size_t>(n));
}

auto load_v(Animations &a, nngn::lua::table_view t, nngn::lua::state_view lua) {
    auto ret = lua.create_table(1, 0);
    for(auto [i, x] : ipairs(t))
        ret.raw_set(i, a.load(x));
    return ret.release();
}

auto remove_v(Animations &a, nngn::lua::table_view t) {
    for(auto [_, x] : ipairs(t))
        a.remove(x.get<Animation*>());
}

void register_function(nngn::lua::table_view t) {
    t["LINEAR"] = AnimationFunction::Type::LINEAR;
    t["RANDOM_F"] = AnimationFunction::Type::RANDOM_F;
    t["RANDOM_3F"] = AnimationFunction::Type::RANDOM_3F;
}

void register_sprite(nngn::lua::table_view t) {
    t["cur_track"] = get<&SpriteAnimation::cur_track>;
    t["track_count"] = get<&SpriteAnimation::track_count>;
    t["set_track"] = set<&SpriteAnimation::set_track>;
}

void register_animation(nngn::lua::table_view t) {
    t["SIZEOF_SPRITE"] =
        nngn::narrow<lua_Integer>(sizeof(nngn::SpriteAnimation));
    t["SIZEOF_LIGHT"] = nngn::narrow<lua_Integer>(sizeof(nngn::LightAnimation));
    t["sprite"] = [](Animation *a) { return static_cast<SpriteAnimation*>(a); };
}

void register_animations(nngn::lua::table_view t) {
    t["max"] = get<&Animations::max>;
    t["max_sprite"] = get<&Animations::max_sprite>;
    t["max_light"] = get<&Animations::max_light>;
    t["n"] = get<&Animations::n>;
    t["n_sprite"] = get<&Animations::n_sprite>;
    t["n_light"] = get<&Animations::n_light>;
    t["set_max"] = set<&Animations::set_max>;
    t["load"] = &Animations::load;
    t["load_v"] = load_v;
    t["remove"] = &Animations::remove;
    t["remove_v"] = remove_v;
}

}

NNGN_LUA_DECLARE_USER_TYPE(AnimationFunction)
NNGN_LUA_DECLARE_USER_TYPE(Animations)
NNGN_LUA_PROXY(AnimationFunction, register_function)
NNGN_LUA_PROXY(SpriteAnimation, register_sprite)
NNGN_LUA_PROXY(Animation, register_animation)
NNGN_LUA_PROXY(Animations, register_animations)
