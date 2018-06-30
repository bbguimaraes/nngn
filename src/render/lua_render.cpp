#include <sol/state_view.hpp>
#include <sol/usertype_proxy.hpp>

#include "luastate.h"

#include "render.h"

using nngn::Renderer;
using nngn::Renderers;

NNGN_LUA_PROXY(Renderer,
    sol::no_constructor,
    "SIZEOF_SPRITE", sol::var(sizeof(nngn::SpriteRenderer)),
    "SIZEOF_CUBE", sol::var(sizeof(nngn::CubeRenderer)),
    "SIZEOF_VOXEL", sol::var(sizeof(nngn::VoxelRenderer)),
    "SPRITE", sol::var(Renderer::Type::SPRITE),
    "CUBE", sol::var(Renderer::Type::CUBE),
    "VOXEL", sol::var(Renderer::Type::VOXEL),
    "z_off", [](const Renderer &r) { return r.z_off; })
NNGN_LUA_PROXY(Renderers,
    sol::no_constructor,
    "RECT", sol::var(Renderers::Debug::RECT),
    "N_DEBUG", sol::var(Renderers::Debug::N_DEBUG),
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
    "set_max_text", &Renderers::set_max_text,
    "set_debug", &Renderers::set_debug,
    "set_perspective", &Renderers::set_perspective,
    "load", &Renderers::load,
    "remove", &Renderers::remove)
