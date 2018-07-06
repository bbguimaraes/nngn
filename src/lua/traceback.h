/**
 * \file
 * \brief Functions for printing the call stack.
 */
#ifndef NNGN_LUA_TRACEBACK_H
#define NNGN_LUA_TRACEBACK_H

#include <ostream>

struct lua_State;

namespace nngn::lua {

/**
 * Type used to output the Lua call stack.
 * \see nngn::lua::print_traceback
 * \see nngn::lua::state_view::print_traceback
 */
class traceback {
public:
    /** Initializes an object, stores the state. */
    explicit traceback(lua_State *L_) noexcept : L{L_} {}
    /** Creates a back trace as a `string` object. */
    std::string str(void) const;
    /** Writes the back trace to an output stream. */
    friend std::ostream &operator<<(std::ostream &os, const traceback &t);
private:
    lua_State *L;
};

}

#endif
