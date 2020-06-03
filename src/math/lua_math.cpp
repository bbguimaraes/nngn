#include <ElysianLua/elysian_lua_thread.hpp>
#include "../xxx_elysian_lua_push_int.h"
#include "../xxx_elysian_lua_as_table.h"
#include <sol/state_view.hpp>
#include <sol/usertype_proxy.hpp>
#include "../xxx_elysian_lua_push_sol_table.h"

#include "luastate.h"

#include "hash.h"
#include "math.h"

using M = std::vector<float>;
using nngn::Math;

namespace {

nngn::Hash hash(std::string_view s) { return nngn::hash(s); }

auto gaussian_filter_v(size_t size, float std_dev) {
    std::vector<std::byte> ret(size * size * sizeof(float));
    Math::gaussian_filter(
        size, std_dev,
        static_cast<float*>(static_cast<void*>(ret.data())));
    return ret;
}

auto gaussian_filter(sol::this_state sol, size_t size, float std_dev) {
    return as_table(
        elysian::lua::ThreadView(sol),
        gaussian_filter_v(size, std_dev));
}

template<typename D>
void fill_rnd_vec_common(
        Math &m, size_t off, size_t n, std::vector<std::byte> &v) {
    using T = typename D::result_type;
    assert(off + n <= v.size() / sizeof(T));
    const auto b = off + static_cast<T*>(static_cast<void*>(v.data()));
    const auto e = b + n;
    auto &gen = *m.rnd_generator();
    auto d = D();
    std::generate(b, e, [&gen, &d] { return d(gen); });
}

void fill_rnd_vec(Math &m, size_t off, size_t n, std::vector<std::byte> &v) {
    fill_rnd_vec_common<
        std::uniform_int_distribution<unsigned char>
    >(m, off, n, v);
}

void fill_rnd_vecf(Math &m, size_t off, size_t n, std::vector<std::byte> &v) {
    fill_rnd_vec_common<std::uniform_real_distribution<float>>(m, off, n, v);
}

auto mat_mul(size_t n, const M &m0, const M &m1, M &m2)
    { Math::mat_mul(n, m0.data(), m1.data(), m2.data()); }

auto rand_float(Math &m) {
    using D = std::uniform_real_distribution<lua_Number>;
    return D{}(*m.rnd_generator());
}

auto rand_int(
        Math &m, std::optional<lua_Integer> v0,
        std::optional<lua_Integer> v1) {
    using D = std::uniform_int_distribution<lua_Integer>;
    auto d = !v0 ? D{} : !v1 ? D(1, *v0) : D(*v0, *v1);
    return d(*m.rnd_generator());
}

auto rand_mat(Math &m, size_t n) {
    M v(n * n);
    m.rand_mat(n, v.data());
    return v;
}

}

NNGN_LUA_PROXY(Math,
    sol::no_constructor,
    "INFINITY", sol::var(INFINITY),
    "FLOAT_EPSILON", sol::var(std::numeric_limits<float>::epsilon()),
    "DOUBLE_EPSILON", sol::var(std::numeric_limits<double>::epsilon()),
    "hash", hash,
    "gaussian_filter_v", gaussian_filter_v,
    "gaussian_filter", gaussian_filter,
    "vec", [](size_t n) { return std::vector<std::byte>(n); },
    "vecf", [](size_t n) { return std::vector<float>(n); },
    "fill_rnd_vec", fill_rnd_vec,
    "fill_rnd_vecf", fill_rnd_vecf,
    "mat_mul", mat_mul,
    "seed_rand", [](Math &m, Math::rand_seed_t s) { m.seed_rand(s); },
    "rand", rand_float,
    "rand_int", rand_int,
    "rand_mat", rand_mat)
