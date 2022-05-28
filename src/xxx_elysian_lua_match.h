#include <ElysianLua/elysian_lua_thread_view.hpp>

#include "lua/state.h"

namespace {

template<typename F, typename T>
bool match_impl(
    const elysian::lua::ThreadViewBase *thread,
    F &&f, void (F::*)(T) const
) {
    using DT = std::decay_t<T>;
    return thread->checkType<DT>(-1)
        && (f(thread->toValue<DT>(-1)), true);
}

template<typename T, typename ...Fs>
void match(nngn::lua::state_view L, const T &t, Fs &&...fs) {
    const elysian::lua::ThreadView thread = {L};
    const int n = thread.push(t);
    (match_impl(&thread, std::forward<Fs>(fs), &Fs::operator()) || ...);
    thread.pop(n);
}

}
