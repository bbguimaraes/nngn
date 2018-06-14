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

auto dump(const Textures &t, nngn::lua::state_arg lua_) {
    const auto lua = nngn::lua::state_view{lua_};
    const auto v = t.dump();
    auto ret = lua.create_table(static_cast<int>(v.size()), 0);
    for(std::size_t i = 0, n = v.size(); i < n; ++i) {
        const auto &[name, count] = v[i];
        auto ti = lua.create_table(2, 0);
        ti.raw_set(1, name);
        ti.raw_set(2, count);
        ret.raw_set(i + 1, ti);
    }
    return ret.release();
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
    "generation", &Textures::generation,
    "set_max", &Textures::set_max,
    "load_data", load_data,
    "load", &Textures::load,
    "reload", &Textures::reload,
    "reload_all", &Textures::reload_all,
    "update_data", update_data,
    "remove", &Textures::remove,
    "dump", dump)
