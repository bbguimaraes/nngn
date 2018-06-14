#include "lua/function.h"
#include "lua/register.h"
#include "lua/table.h"

#include "const.h"
#include "math/lua_vector.h"

#include "texture.h"

using nngn::u32;
using nngn::Textures;

using bvec = nngn::lua_vector<std::byte>;

namespace {

bool write_(const char *filename, const bvec &v, Textures::Format fmt) {
    return Textures::write(filename, v, fmt);
}

void red_to_rgba(bvec *dst, const bvec &src) {
    Textures::red_to_rgba(*dst, src);
}

auto generation(const Textures &t) {
    return nngn::narrow<lua_Integer>(t.generation());
}

auto load_data(Textures &t, const char *name, const bvec *p) {
    return t.load_data(name, p ? p->data() : nullptr);
}

auto update_data(Textures &t, lua_Integer id, const bvec &p) {
    return t.update_data(nngn::narrow<u32>(id), p.data());
}

auto dump(const Textures &t, nngn::lua::state_view lua) {
    const auto v = t.dump();
    const auto n = v.size();
    auto ret = lua.create_table(nngn::narrow<int>(n), 0);
    for(std::size_t i = 0; i != n; ++i) {
        const auto &[name, count] = v[i];
        ret.raw_set(
            nngn::narrow<lua_Integer>(i + 1),
            nngn::lua::table_array(lua, name, count));
    }
    return ret.release();
}

void register_textures(nngn::lua::table_view t) {
    t["SIZE"] = NNGN_TEXTURE_SIZE;
    t["EXTENT"] = NNGN_TEXTURE_EXTENT;
    t["FORMAT_PNG"] = Textures::Format::PNG;
    t["FORMAT_PPM"] = Textures::Format::PPM;
    t["read"] = [](const char *f) { return bvec{Textures::read(f)}; };
    t["write"] = write_;
    t["red_to_rgba"] = red_to_rgba;
    t["flip_y"] = [](bvec *v) { Textures::flip_y(*v); };
    t["max"] = &Textures::max;
    t["n"] = &Textures::n;
    t["generation"] = generation;
    t["set_max"] = &Textures::set_max;
    t["load_data"] = load_data;
    t["load"] = &Textures::load;
    t["reload"] = &Textures::reload;
    t["reload_all"] = &Textures::reload_all;
    t["update_data"] = update_data;
    t["remove"] = &Textures::remove;
    t["dump"] = dump;
}

}

NNGN_LUA_DECLARE_USER_TYPE(Textures)
NNGN_LUA_PROXY(Textures, register_textures)
