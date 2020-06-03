namespace sol {

template<> struct is_stack_based<elysian::lua::StaticStackTable>
    : std::true_type{};

}

namespace elysian::lua {

template<typename Handler> inline bool sol_lua_check(
        sol::types<StaticStackTable>,
        lua_State *L, int index, Handler &&handler,
        sol::stack::record &tracking) {
    const auto t = LuaVM::getMainThread();
    assert(L == t->getState());
    if(t->checkType<StaticStackTable>(index)) {
        tracking.use(1);
        return true;
    }
    handler(L, index, sol::type::table, sol::type_of(L, index), {});
    return false;
}

inline StaticStackTable sol_lua_get(
        sol::types<StaticStackTable>, lua_State *L, int index,
        sol::stack::record &tracking) {
    const auto t = LuaVM::getMainThread();
    assert(L == t->getState());
    tracking.use(1);
    return t->toValue<StaticStackTable>(index);
}

inline int sol_lua_push(
        sol::types<StaticStackTable>,
        lua_State *L, const StaticStackTable &v) {
    const auto t = LuaVM::getMainThread();
    assert(L == t->getState());
    assert(t == v.getThread());
    t->push(v);
    return 1;
}

}
