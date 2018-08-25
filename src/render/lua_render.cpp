#include "lua/state.h"

#include "render.h"

using nngn::Renderer;
using nngn::Renderers;

using nngn::lua::var;

NNGN_LUA_PROXY(Renderer,
    "SIZEOF_SPRITE", var(sizeof(nngn::SpriteRenderer)),
    "SIZEOF_CUBE", var(sizeof(nngn::CubeRenderer)),
    "SPRITE", var(Renderer::Type::SPRITE),
    "CUBE", var(Renderer::Type::CUBE),
    "z_off", [](const Renderer &r) { return r.z_off; })
NNGN_LUA_PROXY(Renderers,
    "DEBUG_RENDERERS", var(Renderers::Debug::DEBUG_RENDERERS),
    "DEBUG_ALL", var(Renderers::Debug::DEBUG_ALL),
    "max_sprites", &Renderers::max_sprites,
    "max_cubes", &Renderers::max_cubes,
    "debug", &Renderers::debug,
    "n", &Renderers::n,
    "n_sprites", &Renderers::n_sprites,
    "n_cubes", &Renderers::n_cubes,
    "set_max_sprites", &Renderers::set_max_sprites,
    "set_max_cubes", &Renderers::set_max_cubes,
    "set_debug", &Renderers::set_debug,
    "load", &Renderers::load,
    "remove", &Renderers::remove)
