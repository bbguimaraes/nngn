namespace {

template<typename T> struct sol_usertype_wrapper {
    T value = {};
    sol_usertype_wrapper() = default;
    sol_usertype_wrapper(T t) : value(t) {}
    T &operator*() { return this->value; }
    auto operator->() { return &this->value; }
    T &get() { return **this; }
};

}

namespace elysian::lua::stack_impl {

template<typename T> struct stack_checker<sol_usertype_wrapper<T>> {
    static bool check(const ThreadViewBase* pBase, StackRecord&, int index) {
        return sol::stack::check_usertype<T>(pBase->getState(), index);
    }
};

template<typename T> struct stack_getter<sol_usertype_wrapper<T>> {
    static sol_usertype_wrapper<T> get(
            const ThreadViewBase* pBase, StackRecord&, int index) {
        return sol_usertype_wrapper<T>(
            sol::stack::get<T>(pBase->getState(), index));
    }
};

template<typename T> struct stack_pusher<sol_usertype_wrapper<T>> {
    static int push(
            const ThreadViewBase* pBase, StackRecord&,
            sol_usertype_wrapper<T> value) {
        return sol::stack::push(pBase->getState(), value.value);
    }
};

}
