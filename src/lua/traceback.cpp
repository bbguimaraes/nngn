#include "traceback.h"

#include <sstream>
#include <string>

#include <lua.hpp>

namespace nngn::lua {

std::ostream &operator<<(std::ostream &os, const traceback &t) {
    luaL_traceback(t.L, t.L, nullptr, 0);
    os << lua_tostring(t.L, -1);
    lua_pop(t.L, 1);
    return os;
}

std::string traceback::str() const {
    std::stringstream s;
    s << *this;
    return s.str();
}

}
