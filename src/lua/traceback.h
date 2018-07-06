#ifndef NNGN_LUA_TRACEBACK_H
#define NNGN_LUA_TRACEBACK_H

#include <ostream>

struct lua_State;

namespace nngn::lua {

class traceback {
public:
    explicit traceback(lua_State *L_) noexcept : L{L_} {}
    std::string str() const;
    friend std::ostream &operator<<(std::ostream &os, const traceback &t);
private:
    lua_State *L;
};

}

#endif
