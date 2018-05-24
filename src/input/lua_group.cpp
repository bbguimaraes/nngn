#include <sol/state_view.hpp>
#include <sol/usertype_proxy.hpp>

#include "luastate.h"

#include "group.h"

using nngn::BindingGroup;

namespace {

void group_add(
    sol::this_state sol, BindingGroup &g, int key, nngn::Input::Selector sel
) { g.add(sol, key, sel); }
void group_remove(sol::this_state sol, BindingGroup &g, int key)
    { g.remove(sol, key); }

}

NNGN_LUA_PROXY(BindingGroup,
    "add", group_add,
    "remove", group_remove)
