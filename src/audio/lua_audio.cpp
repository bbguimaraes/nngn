#include "lua/function.h"
#include "lua/register.h"
#include "lua/table.h"
#include "math/lua_vector.h"
#include "math/math.h"
#include "utils/fixed_string.h"
#include "utils/log.h"

#include "audio.h"
#include "wav.h"

using nngn::i16;
using nngn::Audio;

using bvec = nngn::lua_vector<std::byte>;

enum class pos : std::size_t {};

namespace nngn::lua {

template<>
struct stack_get<pos> {
    static pos get(lua_State *L, int i) {
        const auto n = stack_get<lua_Number>::get(L, i);
        const auto ret = static_cast<pos>(n);
        assert(static_cast<lua_Number>(ret) == std::floor(n));
        return ret;
    }
};

}

NNGN_LUA_DECLARE_USER_TYPE(FILE)
NNGN_LUA_DECLARE_USER_TYPE(nngn::Math, "Math")

namespace {

template<typename T>
auto subspan(bvec *v, pos ib, pos ie) {
    const auto b = static_cast<std::size_t>(ib);
    const auto e = static_cast<std::size_t>(ie);
    const auto ret = nngn::byte_cast<T>(std::span{*v});
    assert(b <= ret.size() && b <= e && e <= ret.size());
    return ret.subspan(b, e - b);
}

template<typename T>
decltype(auto) convert(T &&x) {
    if constexpr(std::is_same_v<T, pos>)
        return static_cast<std::size_t>(x);
    else if constexpr(std::is_same_v<T, lua_Integer>)
        return nngn::narrow<std::size_t>(x);
    else if constexpr(std::is_same_v<T, std::size_t>)
        return nngn::narrow<lua_Integer>(x);
    else
        return FWD(x);
}

template<auto f, typename ...Args>
void static_gen(bvec *v, pos b, pos e, Args ...args) {
    f(subspan<float>(v, b, e), convert(FWD(args))...);
}

template<auto f, typename ...Args>
void gen(const Audio &a, bvec *v, pos b, pos e, Args ...args) {
    (a.*f)(subspan<float>(v, b, e), convert(FWD(args))...);
}

template<auto f, typename ...Args>
auto wrap(Audio &a, lua_Integer i, Args ...args) {
    return convert(
        (a.*f)(nngn::narrow<Audio::source>(i), convert(FWD(args))...));
}

auto db(lua_Number x) {
    return nngn::narrow<lua_Number>(Audio::db(nngn::narrow<float>(x)));
}

void exp_fade(bvec *v, pos b, pos e, pos ep, float bg, float eg, float exp) {
    Audio::exp_fade(
        subspan<float>(v, b, e),
        static_cast<std::size_t>(ep) - static_cast<std::size_t>(b),
        bg, eg, exp);
}

void mix(bvec *v, const bvec &src) {
    Audio::mix(
        nngn::byte_cast<float>(std::span{*v}),
        nngn::byte_cast<const float>(std::span{src}));
}

void normalize(bvec *v, const bvec &src) {
    Audio::normalize(
        nngn::byte_cast<i16>(std::span{*v}),
        nngn::byte_cast<const float>(std::span{src}));
}

bool read_wav(const Audio &a, std::string_view path, bvec *v) {
    return a.read_wav(path, v);
}

bool init(Audio &a, nngn::Math *m, lua_Integer rate) {
    return a.init(m, nngn::narrow<std::size_t>(rate));
}

void wav_header(const Audio &a, bvec *v, const bvec &src) {
    a.gen_wav_header(*v, src);
}

auto n_sources(const Audio &a) {
    return nngn::narrow<lua_Integer>(a.n_sources());
}

bool set_pos(Audio &a, float x, float y, float z) {
    return a.set_pos({x, y, z});
}

std::optional<lua_Integer> add_source(Audio &a, std::optional<const bvec*> v) {
    const auto s = v ? a.add_source(**v) : a.add_source();
    if(const auto ret = static_cast<std::size_t>(s))
        return ret;
    return {};
}

bool set_source_data(
    Audio &a, lua_Integer source, lua_Integer channels, lua_Integer bit_depth,
    const bvec &v)
{
    return a.set_source_data(
        nngn::narrow<Audio::source>(source),
        nngn::narrow<std::size_t>(channels),
        nngn::narrow<std::size_t>(bit_depth),
        v);
}

bool set_source_pos(Audio &a, lua_Integer source, float x, float y, float z) {
    return a.set_source_pos(nngn::narrow<Audio::source>(source), {x, y, z});
}

int write(lua_State *L) {
    const nngn::lua::state_view lua = {L};
    return lua.get<const Audio&>(1).write_wav(
        *nngn::chain_cast<FILE**, void*>(lua.get(2)),
        lua.get<const bvec&>(3));
}

void register_audio(nngn::lua::table_view t) {
    t["db"] = db;
    t["gain"] = static_gen<&Audio::gain, float>;
    t["over"] = static_gen<&Audio::over, float, float>;
    t["fade"] = static_gen<&Audio::fade, float, float>;
    t["exp_fade"] = exp_fade;
    t["env"] = static_gen<&Audio::env, pos, pos, float, pos>;
    t["mix"] = mix;
    t["normalize"] = normalize;
    t["trem"] = gen<&Audio::trem, float, float, float>;
    t["sine"] = gen<&Audio::gen_sine, float>;
    t["sine_fm"] = gen<&Audio::gen_sine_fm, float, float, float, float>;
    t["square"] = gen<&Audio::gen_square, float>;
    t["saw"] = gen<&Audio::gen_saw, float>;
    t["noise"] = gen<&Audio::gen_noise>;
    t["read_wav"] = read_wav;
    t["init"] = init;
    t["wav_header"] = wav_header;
    t["n_sources"] = n_sources;
    t["set_pos"] = set_pos;
    t["add_source"] = add_source;
    t["remove_source"] = wrap<&Audio::remove_source>;
    t["set_source_data"] = set_source_data;
    t["set_source_pos"] = set_source_pos;
    t["set_source_sample_pos"] =
        wrap<&Audio::set_source_sample_pos, lua_Integer>;
    t["set_source_loop"] = wrap<&Audio::set_source_loop, bool>;
    t["set_source_gain"] = wrap<&Audio::set_source_gain, float>;
    t["play"] = wrap<&Audio::play>;
    t["stop"] = wrap<&Audio::stop>;
    t["source_sample_pos"] = wrap<&Audio::source_sample_pos>;
    t["write"] = write;
}

}

NNGN_LUA_DECLARE_USER_TYPE(Audio)
NNGN_LUA_PROXY(Audio, register_audio)
