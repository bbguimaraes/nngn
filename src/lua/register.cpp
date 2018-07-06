#include "register.h"

namespace nngn::lua {

void static_register::register_all(lua_State *L) {
    auto &v = static_register::registry();
    for(auto x : v)
        x(L);
    v.clear();
}

}
