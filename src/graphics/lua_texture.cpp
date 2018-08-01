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

}

NNGN_LUA_PROXY(Textures,
    sol::no_constructor,
    "read", &Textures::read,
    "flip_y", &Textures::flip_y,
    "max", &Textures::max,
    "n", &Textures::n,
    "set_max", &Textures::set_max,
    "load_data", load_data,
    "load", &Textures::load,
    "update_data", update_data)
