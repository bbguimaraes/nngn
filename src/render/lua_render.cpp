#include <sol/state_view.hpp>
#include <sol/usertype_proxy.hpp>

#include "luastate.h"

#include "render.h"

using nngn::Renderer;
using nngn::Renderers;

NNGN_LUA_PROXY(Renderer,
    sol::no_constructor,
    "SIZEOF_SPRITE", sol::var(sizeof(nngn::SpriteRenderer)),
    "SPRITE", sol::var(Renderer::Type::SPRITE),
    "z_off", [](const Renderer &r) { return r.z_off; })
NNGN_LUA_PROXY(Renderers,
    sol::no_constructor,
    "RECT", sol::var(Renderers::Debug::RECT),
    "N_DEBUG", sol::var(Renderers::Debug::N_DEBUG),
    "max_sprites", &Renderers::max_sprites,
    "debug", &Renderers::debug,
    "n", &Renderers::n,
    "n_sprites", &Renderers::n_sprites,
    "set_max_sprites", &Renderers::set_max_sprites,
    "set_debug", &Renderers::set_debug,
    "load", &Renderers::load,
    "remove", &Renderers::remove)
