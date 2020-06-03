#include <type_traits>

namespace {

template<typename From, typename To>
concept PromotesTo = std::is_same_v<std::common_type_t<From, To>, To>;

template<typename T>
concept FitsInteger =
    std::is_same_v<T, size_t>
    || PromotesTo<std::underlying_type_t<T>, lua_Integer>;

}

namespace elysian::lua::stack_impl {

template<FitsInteger T> struct stack_checker<T> {
    static bool check(const ThreadViewBase *L, StackRecord&, int index)
        { return L->isInteger(index); }
};

template<FitsInteger T> struct stack_getter<T> {
    static T get(const ThreadViewBase *L, StackRecord&, int index)
        { return static_cast<T>(L->toInteger(index)); }
};

template<FitsInteger T> struct stack_pusher<T> {
    static int push(const ThreadViewBase *L, StackRecord&, T value)
        { return L->push(static_cast<lua_Integer>(value)); }
};

}
