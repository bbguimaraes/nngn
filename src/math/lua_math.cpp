#include "lua/function.h"
#include "lua/register.h"
#include "lua/table.h"
#include "lua/utils.h"

#include "hash.h"
#include "lua_vector.h"
#include "math.h"

using nngn::Math;
using bvec = nngn::lua_vector<std::byte>;
using fvec = nngn::lua_vector<float>;

namespace {

lua_Integer hash(std::string_view s) {
    using L0 = std::numeric_limits<nngn::Hash>;
    using L1 = std::numeric_limits<std::make_unsigned_t<lua_Integer>>;
    static_assert(L0::digits <= L1::digits);
    return nngn::narrow<lua_Integer>(nngn::hash(s));
}

void gaussian_filter(lua_Integer size, lua_Number std_dev, bvec &v) {
    Math::gaussian_filter(
        nngn::narrow<std::size_t>(size),
        static_cast<float>(std_dev),
        nngn::byte_cast<float>(std::span{v}));
}

void gaussian_filter_f(lua_Integer size, lua_Number std_dev, fvec &v) {
    Math::gaussian_filter(
        nngn::narrow<std::size_t>(size),
        static_cast<float>(std_dev), v);
}

void gaussian_filter2d(
    lua_Integer xsize, lua_Integer ysize, lua_Number std_dev, bvec &v)
{
    Math::gaussian_filter(
        nngn::narrow<std::size_t>(xsize),
        nngn::narrow<std::size_t>(ysize),
        static_cast<float>(std_dev),
        nngn::byte_cast<float>(std::span{v}));
}

void gaussian_filter2d_f(
    lua_Integer xsize, lua_Integer ysize, lua_Number std_dev, fvec &v)
{
    Math::gaussian_filter(
        nngn::narrow<std::size_t>(xsize),
        nngn::narrow<std::size_t>(ysize),
        static_cast<float>(std_dev), v);
}

template<auto f, typename ...Args>
auto vec_fn(nngn::lua::table_view t, Args ...args, nngn::lua::state_view lua) {
    const auto ret = f(nngn::vec3{t[1], t[2], t[3]}, args...);
    return nngn::lua::table_array(
        nngn::lua::state_view{lua},
        nngn::narrow<lua_Number>(ret[0]),
        nngn::narrow<lua_Number>(ret[1]),
        nngn::narrow<lua_Number>(ret[2])
    ).release();
}

template<typename T, typename D>
void fill_rnd_vec_common(
    Math &m, std::size_t off, std::size_t n, std::vector<T> *v)
{
    assert(off + n <= v->size());
    const auto b = off + v->data();
    const auto e = b + n;
    auto &gen = *m.rnd_generator();
    auto d = D();
    std::generate(b, e, [&gen, &d] { return static_cast<T>(d(gen)); });
}

void fill_rnd_vec(Math &m, lua_Integer off, lua_Integer n, bvec *v) {
    using D = std::uniform_int_distribution<unsigned char>;
    fill_rnd_vec_common<std::byte, D>(
        m, nngn::narrow<std::size_t>(off), nngn::narrow<std::size_t>(n), v);
}

void fill_rnd_vecf(Math &m, lua_Integer off, lua_Integer n, fvec *v) {
    using D = std::uniform_real_distribution<float>;
    fill_rnd_vec_common<float, D>(
        m, nngn::narrow<std::size_t>(off), nngn::narrow<std::size_t>(n), v);
}

auto mat_mul(fvec *dst, const fvec &src0, const fvec &src1, lua_Integer n) {
    Math::mat_mul(*dst, src0.data(), src1.data(), nngn::narrow<std::size_t>(n));
}

void seed_rand(Math &m, lua_Integer s) {
     m.seed_rand(nngn::narrow<Math::rand_seed_t>(s));
}

auto rand_float(Math &m) {
    using D = std::uniform_real_distribution<lua_Number>;
    return D{}(*m.rnd_generator());
}

auto int_dist(std::optional<lua_Integer> v0, std::optional<lua_Integer> v1) {
    using D = std::uniform_int_distribution<lua_Integer>;
    return !v0 ? D{} : !v1 ? D{1, *v0} : D{*v0, *v1};
}

auto rand_int(
    Math &m, std::optional<lua_Integer> v0, std::optional<lua_Integer> v1)
{
    return int_dist(v0, v1)(*m.rnd_generator());
}

auto rand_mat(Math &m, lua_Integer n) {
    fvec v(n * n);
    m.rand_mat(v);
    return v;
}

nngn::lua::table_view rand_table_common(
   Math &m, lua_Integer n, auto dist, nngn::lua::state_view lua)
{
    auto &gen = *m.rnd_generator();
    auto ret = lua.create_table(nngn::narrow<int>(n), 0);
    for(std::size_t i = 1; i <= nngn::narrow<std::size_t>(n); ++i)
        ret.raw_set(static_cast<lua_Integer>(i), dist(gen));
    return ret.release();
}

nngn::lua::table_view rand_table(
    Math &m, lua_Integer n,
    std::optional<lua_Integer> v0, std::optional<lua_Integer> v1,
    nngn::lua::state_view lua)
{
    return rand_table_common(m, n, int_dist(v0, v1), lua);
}

nngn::lua::table_view rand_tablef(
    Math &m, lua_Integer n, nngn::lua::state_view lua)
{
    return rand_table_common(
        m, n, std::uniform_real_distribution<lua_Number>{}, lua);
}

void register_math(nngn::lua::table_view t) {
    t["INFINITY"] = nngn::narrow<lua_Number>(INFINITY);
    t["FLOAT_EPSILON"] =
        nngn::narrow<lua_Number>(std::numeric_limits<float>::epsilon());
    t["DOUBLE_EPSILON"] = std::numeric_limits<double>::epsilon();
    t["hash"] = hash;
    t["gaussian_filter"] = gaussian_filter;
    t["gaussian_filter_f"] = gaussian_filter_f;
    t["gaussian_filter2d"] = gaussian_filter2d;
    t["gaussian_filter2d_f"] = gaussian_filter2d_f;
    t["rotate_x"] = vec_fn<
        [](auto v, auto sin, auto cos) { return Math::rotate_x(v, sin, cos); },
        float, float>;
    t["rotate_y"] = vec_fn<
        [](auto v, auto sin, auto cos) { return Math::rotate_y(v, sin, cos); },
        float, float>;
    t["rotate_z"] = vec_fn<
        [](auto v, auto sin, auto cos) { return Math::rotate_z(v, sin, cos); },
        float, float>;
    t["vec"] = [](lua_Integer n) { return bvec(n); };
    t["vecf"] = [](lua_Integer n) { return fvec(n); };
    t["fill_rnd_vec"] = fill_rnd_vec;
    t["fill_rnd_vecf"] = fill_rnd_vecf;
    t["mat_mul"] = mat_mul;
    t["seed_rand"] = seed_rand;
    t["rand"] = rand_float;
    t["rand_int"] = rand_int;
    t["rand_mat"] = rand_mat;
    t["rand_table"] = rand_table;
    t["rand_tablef"] = rand_tablef;
}

}

NNGN_LUA_DECLARE_USER_TYPE(Math)
NNGN_LUA_PROXY(Math, register_math)
