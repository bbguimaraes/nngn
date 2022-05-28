#include <cstddef>
#include <tuple>
#include <utility>

#include <ElysianLua/elysian_lua_thread_view.hpp>

#include "os/platform.h"
#include "utils/utils.h"

template<auto F>
int elysian_lua_wrapper(lua_State *L) {
    return F(elysian::lua::ThreadView(L));
}

/*
int someCFunc(lua_State* pState) {
    ThreadView thread(pState);
    StackFrame frame(
        &thread, requiredArgCount, returnValueCount, optionalArgCount);
    int arg1 = frame.args[0];
    const char* str = frame.args[1];
    Table table;
    if(frame.hasArg(2))
        table = frames.args[2];
    int varArgs = frame.getVarArgCount();
    return frame.returnValues(33, 22.0f, str, arg1, table, thread.stack[-2]);
}
*/

template<typename ...Ts>
class LuaStackFrame {
    static constexpr auto N = static_cast<int>(sizeof...(Ts));
    template<int ...I>
    using idx_seq = std::integer_sequence<int, I...>;
    using make_seq = std::make_integer_sequence<int, N>;
    template<int I>
    using type = std::tuple_element<
        static_cast<std::size_t>(I), std::tuple<Ts...>>;
    elysian::lua::ThreadView L;
    int base() const;
    template<int ...I> auto values(idx_seq<I...>) const;
    template<int ...I> auto check(idx_seq<I...>) const;
public:
    LuaStackFrame(elysian::lua::ThreadView L_) : L(L_) {}
    template<int I> auto value() const;
    auto values() const;
    template<typename ...R> int return_values(R &&...r) const;
};

template<typename ...Ts>
inline int LuaStackFrame<Ts...>::base() const {
    const auto t = this->L.getTop();
    assert(LuaStackFrame::N <= t);
    return t - N + 1;
}

template<typename ...Ts>
template<int ...I>
inline auto LuaStackFrame<Ts...>::check(idx_seq<I...>) const {
    const auto b = this->base();
    return (this->L.checkType<Ts>(b + I) && ...);
}

template<typename ...Ts>
template<int I>
inline auto LuaStackFrame<Ts...>::value() const {
    this->check(LuaStackFrame::idx_seq<I>{});
    return this->L.toValue<LuaStackFrame::type<I>>(I);
}

template<typename ...Ts>
inline auto LuaStackFrame<Ts...>::values() const
    { return this->values(LuaStackFrame::make_seq{}); }

template<typename ...Ts>
template<int ...I>
inline auto LuaStackFrame<Ts...>::values(idx_seq<I...> s) const {
    if constexpr(nngn::Platform::debug)
        this->check(s);
    const auto b = this->base();
    return std::tuple(this->L.toValue<Ts>(b + I)...);
}

template<typename ...Ts>
template<typename ...R>
inline int LuaStackFrame<Ts...>::return_values(R &&...r) const
    { return this->L.push(FWD(r)...); }
