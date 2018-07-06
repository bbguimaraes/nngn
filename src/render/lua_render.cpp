#include "lua/state.h"

#include "render.h"

using nngn::Renderer;
using nngn::Renderers;

using nngn::lua::var;

NNGN_LUA_PROXY(Renderer,
    "SIZEOF_SPRITE", var(sizeof(nngn::SpriteRenderer)),
    "SPRITE", var(Renderer::Type::SPRITE))
NNGN_LUA_PROXY(Renderers,
    "max_sprites", &Renderers::max_sprites,
    "n", &Renderers::n,
    "n_sprites", &Renderers::n_sprites,
    "set_max_sprites", &Renderers::set_max_sprites,
    "load", &Renderers::load,
    "remove", &Renderers::remove)
