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
    t["SIZEOF_CUBE"] = nngn::narrow<lua_Integer>(sizeof(nngn::CubeRenderer));
    t["SPRITE"] = Renderer::Type::SPRITE;
    t["SCREEN_SPRITE"] = Renderer::Type::SCREEN_SPRITE;
    t["CUBE"] = Renderer::Type::CUBE;
    t["z_off"] = z_off;
}

void register_renderers(nngn::lua::table_view t) {
    t["DEBUG_RENDERERS"] = Renderers::Debug::DEBUG_RENDERERS;
    t["DEBUG_ALL"] = Renderers::Debug::DEBUG_ALL;
    t["max_sprites"] = get<&Renderers::max_sprites>;
    t["max_screen_sprites"] = get<&Renderers::max_screen_sprites>;
    t["max_cubes"] = get<&Renderers::max_cubes>;
    t["debug"] = &Renderers::debug;
    t["perspective"] = &Renderers::perspective;
    t["n"] = get<&Renderers::n>;
    t["n_sprites"] = get<&Renderers::n_sprites>;
    t["n_screen_sprites"] = get<&Renderers::n_screen_sprites>;
    t["n_cubes"] = get<&Renderers::n_cubes>;
    t["set_max_sprites"] = set<&Renderers::set_max_sprites>;
    t["set_max_screen_sprites"] = set<&Renderers::set_max_screen_sprites>;
    t["set_max_cubes"] = set<&Renderers::set_max_cubes>;
    t["set_debug"] = &Renderers::set_debug;
    t["set_perspective"] = &Renderers::set_perspective;
    t["load"] = &Renderers::load;
    t["remove"] = &Renderers::remove;
}

}

NNGN_LUA_DECLARE_USER_TYPE(Renderer)
NNGN_LUA_DECLARE_USER_TYPE(Renderers)
NNGN_LUA_PROXY(Renderer, register_renderer)
NNGN_LUA_PROXY(Renderers, register_renderers)
