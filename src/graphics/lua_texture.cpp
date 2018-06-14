#include <sol/state_view.hpp>
#include <sol/usertype_proxy.hpp>

#include "luastate.h"

#include "texture.h"

using nngn::Textures;

namespace {

auto load_data(
        Textures &t, std::string_view name,
        std::optional<const std::vector<std::byte>*> p)
    { return t.load_data(name.data(), p ? (*p)->data() : nullptr); }
auto update_data(
        Textures &t, uint32_t id, const std::vector<std::byte> &p)
    { return t.update_data(id, p.data()); }

auto dump(const sol::this_state sol, const Textures &t) {
    const auto v = t.dump();
    auto ret = sol::stack_table(
        sol, sol::new_table(static_cast<int>(v.size())));
    for(size_t i = 0, n = v.size(); i < n; ++i) {
        const auto &[name, count] = v[i];
        auto ti = sol::stack_table(sol, sol::new_table(2));
        ti.raw_set(1, name);
        ti.raw_set(2, count);
        ret.raw_set(i + 1, ti);
    }
    return ret;
}

}

NNGN_LUA_PROXY(Textures,
    sol::no_constructor,
    "read", &Textures::read,
    "flip_y", &Textures::flip_y,
    "max", &Textures::max,
    "n", &Textures::n,
    "generation", &Textures::generation,
    "set_max", &Textures::set_max,
    "load_data", load_data,
    "load", &Textures::load,
    "reload", &Textures::reload,
    "reload_all", &Textures::reload_all,
    "update_data", update_data,
    "remove", &Textures::remove,
    "dump", dump)
