#ifndef NNGN_MATH_LUA_VECTOR_H
#define NNGN_MATH_LUA_VECTOR_H

#include <cstddef>
#include <vector>

#include "lua/register.h"
#include "lua/user.h"

namespace nngn {

/** Type used to get/push vectors as user data in Lua. */
template<typename T>
class lua_vector : public std::vector<T> {
    using base_type = typename std::vector<T>;
public:
    using base_type::base_type;
    /** Convenience n-element construction from a `lua_Integer`. */
    explicit lua_vector(lua_Integer n)
        : base_type(nngn::narrow<std::size_t>(n)) {}
    explicit lua_vector(base_type &&rhs) : base_type{FWD(rhs)} {}
};

}

namespace nngn::lua {

template<typename T>
struct stack_get<lua_vector<T>*> {
    static lua_vector<T> *get(lua_State *L, int i) {
        return user_data<lua_vector<T>>::get(L, i);
    }
};

template<typename T>
struct stack_get<const lua_vector<T>&> {
    static const lua_vector<T> &get(lua_State *L, int i) {
        return *stack_get<lua_vector<T>*>::get(L, i);
    }
};

template<typename T>
struct stack_push<lua_vector<T>> {
    static int push(lua_State *L, lua_vector<T> v) {
        return user_data<lua_vector<T>>::push(L, std::move(v));
    }
};

}

NNGN_LUA_DECLARE_USER_TYPE(nngn::lua_vector<float>, "vector<float>")
NNGN_LUA_DECLARE_USER_TYPE(nngn::lua_vector<std::byte>, "vector<byte>")

#endif
