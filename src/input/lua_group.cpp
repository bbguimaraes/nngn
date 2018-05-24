#include "lua/function.h"
#include "lua/register.h"
#include "lua/table.h"

#include "group.h"

using nngn::BindingGroup;

namespace {

void group_add(
    BindingGroup &g, int key, nngn::Input::Selector sel,
    nngn::lua::state_view lua)
{
    g.add(lua, key, sel);
}

void group_remove(BindingGroup &g, int key, nngn::lua::state_view lua) {
    g.remove(lua, key);
}

void register_group(nngn::lua::table_view t) {
    t["new"] = [] { return BindingGroup{}; };
    t["next"] = &BindingGroup::next;
    t["set_next"] = &BindingGroup::set_next;
    t["add"] = group_add;
    t["remove"] = group_remove;
}

}

NNGN_LUA_DECLARE_USER_TYPE(BindingGroup)
NNGN_LUA_PROXY(BindingGroup, register_group)
