#include <sol/state_view.hpp>
#include <sol/usertype_proxy.hpp>

#include "luastate.h"

#include "utils/log.h"

#include "compute/thread_pool.h"

namespace {

//sol::optional<size_t> start(nngn::ThreadPool &p, sol::this_state sol) {
int start(lua_State *L0) {
    NNGN_LOG_CONTEXT_F();
    auto &p = **static_cast<nngn::ThreadPool**>(lua_touserdata(L0, 1));
    constexpr int i_func = 2;
//    const auto L0 = static_cast<lua_State*>(sol);
    if(const auto t = lua_type(L0, i_func); t != LUA_TFUNCTION) {
        nngn::Log::l() << "expected function argument, got " << t << '\n';
        return 0;
    }
    const auto top = lua_gettop(L0), n_move = top - 1, n_args = n_move - 1;
    const auto L1 = lua_newthread(L0);
    lua_rotate(L0, -top, 1);
    lua_pushcfunction(L1, &LuaState::msgh);
    lua_xmove(L0, L1, n_move);
    lua_pushinteger(L0, static_cast<lua_Integer>(p.start([L1, n_args] {
        lua_pcall(L1, n_args, 0, 1);
//        lua_close(L1);
    })));
    lua_rotate(L0, -2, 1);
    return 2;
}

}

using nngn::ThreadPool;

NNGN_LUA_PROXY(ThreadPool,
    "sleep_ms", [](std::chrono::milliseconds::rep ms)
        { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); },
    "start", start,
    "join", [](ThreadPool &p, size_t id) { p.thread(id)->join(); })
