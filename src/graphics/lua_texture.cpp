#include "texture.h"

#include "lua/state.h"

using nngn::Textures;

namespace {

bool write_(
    std::string_view filename, const std::vector<std::byte> &v,
    Textures::Format fmt
) {
    return Textures::write(filename.data(), std::span{v}, fmt);
}

auto load_data(
    Textures &t, std::string_view name,
    std::optional<const std::vector<std::byte>*> p
) {
    return t.load_data(name.data(), (p && *p) ? (*p)->data() : nullptr);
}

auto update_data(Textures &t, uint32_t id, const std::vector<std::byte> &p) {
    return t.update_data(id, p.data());
}

}

NNGN_LUA_PROXY(Textures,
    "FORMAT_PNG", nngn::lua::var(Textures::Format::PNG),
    "FORMAT_PPM", nngn::lua::var(Textures::Format::PPM),
    "read", &Textures::read,
    "write", write_,
    "flip_y", &Textures::flip_y,
    "max", &Textures::max,
    "n", &Textures::n,
    "set_max", &Textures::set_max,
    "load_data", load_data,
    "load", &Textures::load,
    "update_data", update_data)
