#include "lua/function.h"
#include "lua/register.h"
#include "lua/table.h"

#include "render.h"

using nngn::Renderer;
using nngn::Renderers;

namespace {

template<auto f>
auto get(const Renderers &r) {
    return nngn::narrow<lua_Integer>((r.*f)());
}

template<auto f>
void set(Renderers &r, lua_Integer n) {
    (r.*f)(nngn::narrow<std::size_t>(n));
}

auto z_off(const Renderer &r) {
    return nngn::narrow<lua_Number>(r.z_off);
}

void register_renderer(nngn::lua::table_view t) {
    t["SIZEOF_SPRITE"] =
        nngn::narrow<lua_Integer>(sizeof(nngn::SpriteRenderer));
    t["SPRITE"] = Renderer::Type::SPRITE;
    t["z_off"] = z_off;
}

void register_renderers(nngn::lua::table_view t) {
    t["DEBUG_RENDERERS"] = Renderers::Debug::DEBUG_RENDERERS;
    t["DEBUG_ALL"] = Renderers::Debug::DEBUG_ALL;
    t["max_sprites"] = get<&Renderers::max_sprites>;
    t["debug"] = &Renderers::debug;
    t["n"] = get<&Renderers::n>;
    t["n_sprites"] = get<&Renderers::n_sprites>;
    t["set_max_sprites"] = set<&Renderers::set_max_sprites>;
    t["set_debug"] = &Renderers::set_debug;
    t["load"] = &Renderers::load;
    t["remove"] = &Renderers::remove;
}

}

NNGN_LUA_DECLARE_USER_TYPE(Renderer)
NNGN_LUA_DECLARE_USER_TYPE(Renderers)
NNGN_LUA_PROXY(Renderer, register_renderer)
NNGN_LUA_PROXY(Renderers, register_renderers)
