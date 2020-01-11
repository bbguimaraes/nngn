#include "lua/state.h"
#include "lua/utils.h"

#include "hash.h"
#include "math.h"

using M = std::vector<float>;
using nngn::Math;

namespace {

nngn::Hash hash(std::string_view s) { return nngn::hash(s); }

template<typename T>
T *vec_data(std::vector<std::byte> *v) {
    return static_cast<T*>(static_cast<void*>((v->data())));
}

void gaussian_filter(
    std::size_t size, float std_dev, std::vector<std::byte> *v
) {
    assert(size * sizeof(float) <= v->size());
    Math::gaussian_filter(size, std_dev, vec_data<float>(v));
}

void gaussian_filter2d(
    std::size_t xsize, std::size_t ysize, float std_dev,
    std::vector<std::byte> *v
) {
    assert(xsize * ysize * sizeof(float) <= v->size());
    Math::gaussian_filter(xsize, ysize, std_dev, vec_data<float>(v));
}

template<auto f, typename ...Args>
auto vec_fn(nngn::lua::table_view t, Args ...args, nngn::lua::state_arg lua) {
    const auto ret = f(nngn::vec3{t[1], t[2], t[3]}, args...);
    return nngn::lua::table_array(
        nngn::lua::state_view{lua}, ret[0], ret[1], ret[2]).release();
}

template<typename D>
void fill_rnd_vec_common(
        Math &m, std::size_t off, std::size_t n, std::vector<std::byte> *v) {
    using T = typename D::result_type;
    assert(off + n <= v->size() / sizeof(T));
    const auto b = off + vec_data<T>(v);
    const auto e = b + n;
    auto &gen = *m.rnd_generator();
    auto d = D();
    std::generate(b, e, [&gen, &d] { return d(gen); });
}

void fill_rnd_vec(
    Math &m, std::size_t off, std::size_t n, std::vector<std::byte> *v
) {
    fill_rnd_vec_common<
        std::uniform_int_distribution<unsigned char>
    >(m, off, n, v);
}

void fill_rnd_vecf(
    Math &m, std::size_t off, std::size_t n, std::vector<std::byte> *v
) {
    fill_rnd_vec_common<std::uniform_real_distribution<float>>(m, off, n, v);
}

auto mat_mul(std::size_t n, const M &m0, const M &m1, M &m2) {
    Math::mat_mul(n, m0.data(), m1.data(), m2.data());
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
    Math &m, std::optional<lua_Integer> v0, std::optional<lua_Integer> v1
) {
    return int_dist(v0, v1)(*m.rnd_generator());
}

auto rand_mat(Math &m, std::size_t n) {
    M v(n * n);
    m.rand_mat(n, v.data());
    return v;
}

auto rand_table_common(
    nngn::lua::state_view lua, Math &m, std::size_t n, auto dist
) {
    auto &gen = *m.rnd_generator();
    auto ret = lua.create_table(static_cast<int>(n), 0);
    for(std::size_t i = 1; i <= n; ++i)
        ret.raw_set(i, dist(gen));
    return ret.release();
}

auto rand_table(
    Math &m, std::size_t n,
    std::optional<lua_Integer> v0, std::optional<lua_Integer> v1,
    nngn::lua::state_arg lua
) {
    return rand_table_common(
        static_cast<lua_State*>(lua), m, n, int_dist(v0, v1));
}

auto rand_tablef(Math &m, std::size_t n, nngn::lua::state_arg lua) {
    return rand_table_common(
        static_cast<lua_State*>(lua), m, n,
        std::uniform_real_distribution<lua_Number>{});
}

}

NNGN_LUA_PROXY(Math,
    "INFINITY", nngn::lua::var(INFINITY),
    "FLOAT_EPSILON", nngn::lua::var(std::numeric_limits<float>::epsilon()),
    "DOUBLE_EPSILON", nngn::lua::var(std::numeric_limits<double>::epsilon()),
    "hash", hash,
    "gaussian_filter", gaussian_filter,
    "gaussian_filter2d", gaussian_filter2d,
    "rotate_x", vec_fn<
        [](auto v, auto sin, auto cos) { return Math::rotate_x(v, sin, cos); },
        float, float>,
    "rotate_y", vec_fn<
        [](auto v, auto sin, auto cos) { return Math::rotate_y(v, sin, cos); },
        float, float>,
    "rotate_z", vec_fn<
        [](auto v, auto sin, auto cos) { return Math::rotate_z(v, sin, cos); },
        float, float>,
    "vec", [](std::size_t n) { return std::vector<std::byte>(n); },
    "vecf", [](std::size_t n) { return std::vector<float>(n); },
    "fill_rnd_vec", fill_rnd_vec,
    "fill_rnd_vecf", fill_rnd_vecf,
    "mat_mul", mat_mul,
    "seed_rand", [](Math &m, Math::rand_seed_t s) { m.seed_rand(s); },
    "rand", rand_float,
    "rand_int", rand_int,
    "rand_mat", rand_mat,
    "rand_table", rand_table,
    "rand_tablef", rand_tablef)
