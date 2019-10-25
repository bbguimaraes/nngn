#include "lua/function.h"
#include "lua/register.h"
#include "lua/table.h"

#include "model.h"
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
    t["SIZEOF_VOXEL"] = nngn::narrow<lua_Integer>(sizeof(nngn::VoxelRenderer));
    t["SPRITE"] = Renderer::Type::SPRITE;
    t["SCREEN_SPRITE"] = Renderer::Type::SCREEN_SPRITE;
    t["TRANSLUCENT"] = Renderer::Type::TRANSLUCENT;
    t["CUBE"] = Renderer::Type::CUBE;
    t["VOXEL"] = Renderer::Type::VOXEL;
    t["MODEL"] = Renderer::Type::MODEL;
    t["MODEL_DEDUP"] = nngn::Models::Flag::DEDUP;
    t["MODEL_CALC_NORMALS"] = nngn::Models::Flag::CALC_NORMALS;
    t["z_off"] = z_off;
}

void register_renderers(nngn::lua::table_view t) {
    t["DEBUG_RENDERERS"] = Renderers::Debug::DEBUG_RENDERERS;
    t["DEBUG_CIRCLE"] = Renderers::Debug::DEBUG_CIRCLE;
    t["DEBUG_BB"] = Renderers::Debug::DEBUG_BB;
    t["DEBUG_DEPTH"] = Renderers::Debug::DEBUG_DEPTH;
    t["DEBUG_ALL"] = Renderers::Debug::DEBUG_ALL;
    t["max_sprites"] = get<&Renderers::max_sprites>;
    t["max_screen_sprites"] = get<&Renderers::max_screen_sprites>;
    t["max_cubes"] = get<&Renderers::max_cubes>;
    t["max_voxels"] = get<&Renderers::max_voxels>;
    t["debug"] = &Renderers::debug;
    t["perspective"] = &Renderers::perspective;
    t["zsprites"] = &Renderers::zsprites;
    t["n"] = get<&Renderers::n>;
    t["n_sprites"] = get<&Renderers::n_sprites>;
    t["n_screen_sprites"] = get<&Renderers::n_screen_sprites>;
    t["n_translucent"] = get<&Renderers::n_translucent>;
    t["n_cubes"] = get<&Renderers::n_cubes>;
    t["n_voxels"] = get<&Renderers::n_voxels>;
    t["selected"] = &Renderers::selected;
    t["set_max_sprites"] = set<&Renderers::set_max_sprites>;
    t["set_max_screen_sprites"] = set<&Renderers::set_max_screen_sprites>;
    t["set_max_translucent"] = set<&Renderers::set_max_translucent>;
    t["set_max_cubes"] = set<&Renderers::set_max_cubes>;
    t["set_max_voxels"] = set<&Renderers::set_max_voxels>;
    t["set_max_text"] = set<&Renderers::set_max_text>;
    t["set_max_colliders"] = set<&Renderers::set_max_colliders>;
    t["set_debug"] = &Renderers::set_debug;
    t["set_perspective"] = &Renderers::set_perspective;
    t["set_zsprites"] = &Renderers::set_zsprites;
    t["load"] = &Renderers::load;
    t["remove"] = &Renderers::remove;
    t["add_selection"] = &Renderers::add_selection;
    t["remove_selection"] = &Renderers::remove_selection;
}

}

NNGN_LUA_DECLARE_USER_TYPE(Renderer)
NNGN_LUA_DECLARE_USER_TYPE(Renderers)
NNGN_LUA_PROXY(Renderer, register_renderer)
NNGN_LUA_PROXY(Renderers, register_renderers)
