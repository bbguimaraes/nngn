#include "lua.h"

#include <cmath>

#include <lua.hpp>

#include "utils/log.h"

#include "state.h"
#include "traceback.h"
#include "value.h"

namespace nngn::lua {

int msgh(lua_State *L) {
    NNGN_LOG_CONTEXT_F();
    const char *s = lua_tostring(L, -1);
    if(!s) {
        luaL_tolstring(L, -1, nullptr);
        s = lua_tostring(L, -1);
    }
    Log::l() << s << ", " << traceback{L} << '\n';
    return 0;
}

void print_stack(lua_State *L) {
    NNGN_LOG_CONTEXT_F();
    auto &l = Log::l();
    const state_view lua = {L};
    const int n = lua.top();
    l << "top: " << n << '\n';
    for(int i = n; i; --i) {
        const auto t = lua.get_type(i);
        const auto [_, s] = lua.to_string(i);
        l << "  " << i << ' ' << type_str(t);
        switch(t) {
        case type::function:
            if(lua_iscfunction(lua, i)) {
                l << " (c" << s << ')';
                break;
            }
            [[fallthrough]];
        case type::none:
        case type::nil:
        case type::boolean:
        case type::light_user_data:
        case type::number:
        case type::string:
        case type::table:
        case type::user_data:
        case type::thread:
        default:
            l << " (" << s << ')';
        }
        l << '\n';
    }
}

void print_traceback(lua_State *L) {
    Log::l() << traceback{L} << '\n';
}

}
