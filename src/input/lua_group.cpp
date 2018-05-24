#include "lua/state.h"

#include "group.h"

using nngn::BindingGroup;

namespace {

void group_add(
    BindingGroup &g, int key, nngn::Input::Selector sel,
    nngn::lua::state_arg lua
) {
    g.add(lua, key, sel);
}

void group_remove(BindingGroup &g, int key, nngn::lua::state_arg lua) {
    g.remove(lua, key);
}

}

NNGN_LUA_PROXY(BindingGroup,
    "add", group_add,
    "remove", group_remove)
