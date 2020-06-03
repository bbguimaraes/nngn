#include <string_view>

#include <lua.hpp>
#include <ElysianLua/elysian_lua_table_proxy.hpp>
#include <ElysianLua/elysian_lua_thread.hpp>
#include "../xxx_elysian_lua_push_int.h"
#include "../xxx_elysian_lua_string_view.h"
#include <sol/state_view.hpp>
#include <sol/usertype_proxy.hpp>
#include "../xxx_elysian_lua_push_sol_table.h"

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
    const elysian::lua::ThreadView el(sol);
    const auto ret = el.createTable(static_cast<int>(v.size()));
    for(size_t i = 0, n = v.size(); i < n; ++i) {
        const auto &[name, count] = v[i];
        ret.setFieldRaw(i, el.createTable(elysian::lua::LuaTableValues{
            elysian::lua::LuaPair{1, name},
            elysian::lua::LuaPair{2, count}}));
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
