#include <sol/state_view.hpp>
#include <sol/usertype_proxy.hpp>

#include "luastate.h"

#include "render.h"

using nngn::Renderer;
using nngn::Renderers;

NNGN_LUA_PROXY(Renderer,
    sol::no_constructor,
    "SIZEOF_SPRITE", sol::var(sizeof(nngn::SpriteRenderer)),
    "SPRITE", sol::var(Renderer::Type::SPRITE))
NNGN_LUA_PROXY(Renderers,
    sol::no_constructor,
    "max_sprites", &Renderers::max_sprites,
    "n", &Renderers::n,
    "n_sprites", &Renderers::n_sprites,
    "set_max_sprites", &Renderers::set_max_sprites,
    "load", &Renderers::load,
    "remove", &Renderers::remove)
