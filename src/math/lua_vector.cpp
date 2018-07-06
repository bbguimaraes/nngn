#include "lua/function.h"
#include "lua/register.h"
#include "lua/table.h"

#include "lua_vector.h"

using bvec = nngn::lua_vector<std::byte>;
using fvec = nngn::lua_vector<float>;

namespace {

template<typename T>
auto vec_len(const nngn::lua_vector<T> &v) {
    return nngn::narrow<lua_Integer>(v.size());
}

template<typename T>
auto vec_index(const nngn::lua_vector<T> &v, lua_Integer li) {
    constexpr auto is_float = std::same_as<T, float>;
    using O = std::optional<std::conditional_t<is_float, lua_Number, T>>;
    if(const auto i = nngn::narrow<std::size_t>(li - 1); i < v.size()) {
        if constexpr(is_float)
            return O{nngn::narrow<lua_Number>(v[i])};
        else
            return O{v[i]};
    }
    return O{};
}

template<typename T, nngn::fixed_string name>
nngn::lua::value_view vec_to_string(
    const nngn::lua_vector<T> &v, nngn::lua::state_view lua)
{
    constexpr auto push = [](auto &l, auto x) {
        if constexpr(std::same_as<float, T>)
            l.push(nngn::narrow<lua_Number>(x));
        else
            l.push(x);
    };
    luaL_Buffer b = {};
    luaL_buffinit(lua, &b);
    luaL_addstring(&b, "vector<");
    luaL_addlstring(&b, name.data(), name.size());
    lua_pushfstring(lua, ">: %p {", v.data());
    luaL_addvalue(&b);
    if(!v.empty()) {
        push(lua, v[0]);
        luaL_addvalue(&b);
        for(auto i = begin(v), e = end(v); ++i != e;) {
            luaL_addstring(&b, ", ");
            push(lua, *i);
            luaL_addvalue(&b);
        }
    }
    luaL_addstring(&b, "}");
    luaL_pushresult(&b);
    return {lua, lua.top()};
}

template<typename T, nngn::fixed_string name>
void register_vector(nngn::lua::table_view t) {
    t["__tostring"] = vec_to_string<T, name>;
    t["__len"] = vec_len<T>;
    t["__index"] = vec_index<T>;
}

}

NNGN_LUA_PROXY(fvec, (register_vector<float, "float">))
NNGN_LUA_PROXY(bvec, (register_vector<std::byte, "byte">))
