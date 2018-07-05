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
    "DEBUG_ALL", var(Renderers::Debug::DEBUG_ALL),
    "max_sprites", &Renderers::max_sprites,
    "max_cubes", &Renderers::max_cubes,
    "max_voxels", &Renderers::max_voxels,
    "debug", &Renderers::debug,
    "perspective", &Renderers::perspective,
    "n", &Renderers::n,
    "n_sprites", &Renderers::n_sprites,
    "n_cubes", &Renderers::n_cubes,
    "n_voxels", &Renderers::n_voxels,
    "set_max_sprites", &Renderers::set_max_sprites,
    "set_max_cubes", &Renderers::set_max_cubes,
    "set_max_voxels", &Renderers::set_max_voxels,
    "set_debug", &Renderers::set_debug,
    "set_perspective", &Renderers::set_perspective,
    "load", &Renderers::load,
    "remove", &Renderers::remove)
