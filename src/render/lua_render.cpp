#include "lua/state.h"

#include "render.h"

using nngn::Renderer;
using nngn::Renderers;

using nngn::lua::var;

NNGN_LUA_PROXY(Renderer,
    "SIZEOF_SPRITE", var(sizeof(nngn::SpriteRenderer)),
    "SIZEOF_CUBE", var(sizeof(nngn::CubeRenderer)),
    "SIZEOF_VOXEL", var(sizeof(nngn::VoxelRenderer)),
    "SPRITE", var(Renderer::Type::SPRITE),
    "CUBE", var(Renderer::Type::CUBE),
    "VOXEL", var(Renderer::Type::VOXEL),
    "z_off", [](const Renderer &r) { return r.z_off; })
NNGN_LUA_PROXY(Renderers,
    "DEBUG_RENDERERS", var(Renderers::Debug::DEBUG_RENDERERS),
    "DEBUG_CIRCLE", var(Renderers::Debug::DEBUG_CIRCLE),
    "DEBUG_BB", var(Renderers::Debug::DEBUG_BB),
    "DEBUG_LIGHT", var(Renderers::Debug::DEBUG_LIGHT),
    "DEBUG_DEPTH", var(Renderers::Debug::DEBUG_DEPTH),
    "DEBUG_ALL", var(Renderers::Debug::DEBUG_ALL),
    "max_sprites", &Renderers::max_sprites,
    "max_cubes", &Renderers::max_cubes,
    "max_voxels", &Renderers::max_voxels,
    "debug", &Renderers::debug,
    "perspective", &Renderers::perspective,
    "zsprites", &Renderers::zsprites,
    "n", &Renderers::n,
    "n_sprites", &Renderers::n_sprites,
    "n_cubes", &Renderers::n_cubes,
    "n_voxels", &Renderers::n_voxels,
    "selected", &Renderers::selected,
    "set_max_sprites", &Renderers::set_max_sprites,
    "set_max_cubes", &Renderers::set_max_cubes,
    "set_max_voxels", &Renderers::set_max_voxels,
    "set_max_text", &Renderers::set_max_text,
    "set_debug", &Renderers::set_debug,
    "set_perspective", &Renderers::set_perspective,
    "set_zsprites", &Renderers::set_zsprites,
    "set_max_colliders", &Renderers::set_max_colliders,
    "load", &Renderers::load,
    "remove", &Renderers::remove,
    "add_selection", &Renderers::add_selection,
    "remove_selection", &Renderers::remove_selection)
