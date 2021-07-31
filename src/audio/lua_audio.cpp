#include "lua/state.h"
#include "lua/utils.h"
#include "math/math.h"
#include "os/platform.h"
#include "utils/fixed_string.h"
#include "utils/log.h"

#include "audio.h"
#include "wav.h"

using nngn::i16, nngn::u32;
using nngn::Audio;

namespace {

template<typename T> constexpr nngn::fixed_string metatable_name = {""};
template<> constexpr nngn::fixed_string metatable_name<Audio> = {"Audio"};

template<typename F, nngn::member_function_pointer auto f>
struct member_fn_impl;

#define D(CONST) \
    template< \
        typename T, typename R, typename ...Args, \
        /*TODO std::same_as<R(T::*)(Args...) CONST>*/ auto f \
    > \
    struct member_fn_impl<R(T::*)(Args...) CONST, f> { \
        static decltype(auto) call( \
            sol::lightuserdata_value u, Args ...args, sol::this_state L \
        ) { \
            return (to_userdata<T>(L, 1, u.value)->*f)(FWD(args)...); \
        } \
    };
D()
D(const)
#undef D

template<nngn::member_function_pointer auto f>
constexpr auto member_fn = &member_fn_impl<decltype(f), f>::call;

[[maybe_unused]]
bool check_type(lua_State *L, int i, const char *meta) {
    NNGN_LOG_CONTEXT_F();
    NNGN_ANON_DECL(nngn::lua::stack_mark(L));
    if(!lua_getmetatable(L, i)) {
        nngn::Log::l()
            << "failed to check for " << meta
            << ", object has no meta table\n";
        return false;
    }
    lua_getglobal(L, meta);
    if(!lua_compare(L, -2, -1, LUA_OPEQ)) {
        lua_pushvalue(L, i);
        std::size_t len = 0;
        const char *s = luaL_tolstring(L, -1, &len);
        nngn::Log::l()
            << "value at index " << i << " is "
            << std::string_view{s, len} << ", not " << meta << '\n';
        lua_pop(L, 3);
        return false;
    }
    lua_pop(L, 2);
    return true;
}

template<typename T>
T *to_userdata(lua_State *L, int i, void *p) {
    (void)L, (void)i;
    if constexpr(nngn::Platform::debug)
        if(!check_type(L, i, metatable_name<T>.data()))
            return nullptr;
    return static_cast<T*>(p);
}

template<typename T, nngn::fixed_string name>
int construct(lua_State *L) {
    NNGN_LOG_CONTEXT_F();
    NNGN_ANON_DECL(nngn::lua::stack_mark(L, 1));
    lua_newuserdatauv(L, sizeof(T), 0);
    new (lua_touserdata(L, -1)) T;
    lua_getglobal(L, name.data());
    lua_setmetatable(L, -2);
    return 1;
}

template<typename T>
int destruct(lua_State *L) {
    static_cast<T*>(lua_touserdata(L, -1))->~T();
    return 0;
}

template<typename T>
using byte_arg_type =
    std::conditional_t<std::is_const_v<T>, const std::byte, std::byte>;

template<typename T>
auto to_span(std::span<byte_arg_type<T>> s) {
    assert(!(s.size() % sizeof(T)));
    return std::span{nngn::byte_cast<T*>(s.data()), s.size() / sizeof(T)};
}

template<typename T>
auto subspan(std::vector<std::byte> *v, std::size_t b, std::size_t e) {
    const auto ret = to_span<T>(std::span{*v});
    assert(b <= ret.size() && b <= e && e <= ret.size());
    return ret.subspan(b, e - b);
}

bool set_pos(
    sol::lightuserdata_value u, float x, float y, float z,
    sol::this_state L
) {
    auto *const a = to_userdata<Audio>(L, 1, u.value);
    return a->set_pos({x, y, z});
}

auto add_source(
    sol::lightuserdata_value u, const std::vector<std::byte> &v,
    sol::this_state L
) {
    auto *const a = to_userdata<Audio>(L, 1, u.value);
    const auto ret = a->add_source(v);
    return ret ? std::optional<std::size_t>{ret} : std::nullopt;
}

void exp_fade(
    std::vector<std::byte> &v, std::size_t b, std::size_t e, std::size_t ep,
    float g0, float g1, float exp
) {
    Audio::exp_fade(subspan<float>(&v, b, e), ep - b, g0, g1, exp);
}

void mix(std::vector<std::byte> &dst, const std::vector<std::byte> &src) {
    Audio::mix(to_span<float>(dst), to_span<const float>(src));
}

void normalize(std::vector<std::byte> &dst, const std::vector<std::byte> &src) {
    Audio::normalize(to_span<i16>(dst), to_span<const float>(src));
}

template<auto f, typename ...Args>
void static_gen(
    std::vector<std::byte> &v, std::size_t b, std::size_t e, Args &&...args
) {
    f(subspan<float>(&v, b, e), FWD(args)...);
}

template<auto f, typename ...Args>
void gen(
    sol::lightuserdata_value u,
    std::vector<std::byte> &v, std::size_t b, std::size_t e, Args &&...args,
    nngn::lua::state_arg lua
) {
    const auto *const a = to_userdata<Audio>(lua, 1, u.value);
    (a->*f)(subspan<float>(&v, b, e), FWD(args)...);
}

bool set_source_pos(
    sol::lightuserdata_value u, std::size_t source, float x, float y, float z,
    sol::this_state L
) {
    auto *const a = to_userdata<Audio>(L, 1, u.value);
    return a->set_source_pos(source, {x, y, z});
}

void wav_header(
    sol::lightuserdata_value u,
    std::vector<std::byte> *dst, const std::vector<std::byte> &src,
    nngn::lua::state_arg lua
) {
    assert(44 <= dst->size());
    const auto *const a = to_userdata<Audio>(lua, 1, u.value);
    a->gen_wav_header(*dst, src);
}

bool write(
    sol::this_state L, sol::lightuserdata_value u, sol::lightuserdata_value f,
    const std::vector<std::byte> &v
) {
    const auto *const a = to_userdata<Audio>(L, 1, u.value);
    return a->write_wav(static_cast<FILE*>(f.value), v);
}

auto NNGN_LUA_REGISTER_AUDIO_F = [](lua_State *L) {
    NNGN_LOG_CONTEXT("NNGN_LUA_REGISTER_AUDIO_F");
    NNGN_ANON_DECL(nngn::lua::stack_mark(L));
    sol::state_view sol = L;
    lua_newtable(L);
    sol::stack_table t = {L, lua_gettop(L)};
    t["__index"] = t;
    t["__name"] = "Audio";
    (void)destruct<Audio>; // t["__gc"] = destruct;
    t["db"] = &Audio::db;
    t["gain"] = static_gen<&Audio::gain, float>;
    t["fade"] = static_gen<&Audio::fade, float, float>;
    t["exp_fade"] = exp_fade,
    t["env"] =
        static_gen<&Audio::env, std::size_t, std::size_t, float, std::size_t>;
    t["mix"] = mix;
    t["normalize"] = normalize;
    t["sine"] = gen<&Audio::gen_sine, float>;
    t["sine_fm"] = gen<&Audio::gen_sine_fm, float, float, float, float>;
    t["square"] = gen<&Audio::gen_square, float>;
    t["saw"] = gen<&Audio::gen_saw, float>;
    t["noise"] = gen<&Audio::gen_noise>;
    t["read_wav"] = &Audio::read_wav;
    t["new"] = construct<Audio, "Audio">;
    t["init"] = member_fn<&Audio::init>;
    t["wav_header"] = wav_header;
    t["n_sources"] = member_fn<&Audio::n_sources>;
    t["set_pos"] = set_pos;
    t["add_source"] = add_source;
    t["remove_source"] = member_fn<&Audio::remove_source>;
    t["set_source_pos"] = set_source_pos;
    t["play"] = member_fn<&Audio::play>;
    t["stop"] = member_fn<&Audio::stop>;
    t["source_sample_pos"] = member_fn<&Audio::source_sample_pos>;
    t["write"] = write;
    sol["Audio"] = t;
    lua_pop(L, 1);
};

auto NNGN_LUA_REGISTER_AUDIO =
    nngn::lua::static_register{NNGN_LUA_REGISTER_AUDIO_F};

}
