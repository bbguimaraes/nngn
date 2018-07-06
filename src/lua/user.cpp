#include "user.h"

#include "os/platform.h"
#include "utils/log.h"

#include "state.h"
#include "table.h"
#include "utils.h"

namespace {

nngn::lua::table push_metatable(
    nngn::lua::state_view lua, std::string_view meta)
{
    NNGN_ANON_DECL(nngn::lua::stack_mark(lua, 1));
    nngn::lua::table ret = nngn::lua::global_table{lua}[meta];
    if constexpr(nngn::Platform::debug)
        if(const auto t = ret.get_type(); t != nngn::lua::type::table)
            nngn::Log::l()
                << "attempted to push meta table " << std::quoted(meta)
                << " but got a " << type_str(t)
                << " value (" << std::quoted(ret.to_string().second) << ")\n";
    return ret;
}

bool check_type(
    nngn::lua::state_view lua, int i, std::string_view meta,
    nngn::lua::type t)
{
    NNGN_ANON_DECL(stack_mark(lua));
    const auto log = [meta, i](void) -> decltype(auto) {
        return nngn::Log::l()
            << "expected a user data value of type " << std::quoted(meta)
            << " at index " << i;
    };
    if(t != nngn::lua::type::user_data) {
        log() << ", found " << type_str(t) << '\n';
        return false;
    }
    if(!lua_getmetatable(lua, i)) {
        log() << ", found user data with no meta table\n";
        return false;
    }
    push_metatable(lua, meta).release();
    NNGN_ANON_DECL(nngn::lua::defer_pop(lua, 2));
    if(!lua_compare(lua, -2, -1, LUA_OPEQ)) {
        log()
            << ", found user data of type "
            << std::quoted(lua.to_string(-1).second) << '\n';
        return false;
    }
    return true;
}

}

namespace nngn::lua::detail {

table user_data_base::push_metatable(state_view lua, std::string_view meta) {
    NNGN_LOG_CONTEXT_CF(user_data_base);
    return ::push_metatable(lua, meta);
}

bool user_data_base::check_type(
    state_view lua, int i, std::string_view meta)
{
    NNGN_LOG_CONTEXT_CF(user_data_base);
    return ::check_type(lua, i, meta, lua.get_type(i));
}

bool user_data_base::check_pointer_type(
    state_view lua, int i, std::string_view meta)
{
    NNGN_LOG_CONTEXT_CF(user_data_base);
    const auto t = lua.get_type(i);
    return t == type::none
        || t == type::nil
        || ::check_type(lua, i, meta, t);
}

}
