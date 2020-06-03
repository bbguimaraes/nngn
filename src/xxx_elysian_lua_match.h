namespace {

template<typename F, typename T>
bool match_impl(
        const elysian::lua::ThreadViewBase *thread,
        F &&f, void (F::*)(T) const) {
    using DT = std::decay_t<T>;
    return thread->checkType<DT>(-1)
        && (f(thread->toValue<DT>(-1)), true);
}

template<typename T, typename ...Fs>
void match(const T &t, Fs &&...fs) {
    const auto thread = t.getThread();
    const int n = thread->push(t);
    (match_impl(thread, std::forward<Fs>(fs), &Fs::operator()) || ...);
    thread->pop(n);
}

}
