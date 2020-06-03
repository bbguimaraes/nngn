namespace elysian::lua::stack_impl {

template<>
struct stack_checker<std::string_view> {
    static bool check(const ThreadViewBase* pBase, StackRecord&, int index) {
        return pBase->isString(index);
    }
};

template<>
struct stack_getter<std::string_view> {
    static std::string_view get(
            const ThreadViewBase* pBase, StackRecord&, int index) {
        size_t len;
        const auto ret = pBase->toString(index, &len);
        return {ret, len};
    }
};

template<> struct stack_pusher<std::string_view> {
    static int push(
            const ThreadViewBase* pBase, StackRecord&,
            std::string_view value) {
        lua_pushlstring(*pBase, value.data(), value.size());
        return 1;
    }
};

}
