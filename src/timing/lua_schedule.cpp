#include <sol/state_view.hpp>
#include <sol/usertype_proxy.hpp>

#include "luastate.h"

#include "utils/def.h"
#include "utils/delegate.h"
#include "utils/log.h"
#include "utils/scoped.h"

#include "schedule.h"

using nngn::Schedule;

namespace {

struct entry { lua_State *L = {}; int ref = {}; };

bool call(void *p) {
    NNGN_LOG_CONTEXT_F();
    const auto *e = static_cast<const entry*>(p);
    lua_pushcfunction(e->L, LuaState::msgh);
    int msgh = lua_gettop(e->L);
    lua_rawgeti(e->L, LUA_REGISTRYINDEX, e->ref);
    const auto pop = nngn::make_scoped_obj(
        e->L, [](auto *L) { lua_pop(L, 1); });
    return lua_pcall(e->L, 0, 0, msgh) == LUA_OK;
}

bool destroy(void *p) {
    NNGN_LOG_CONTEXT_F();
    const auto *e = static_cast<const entry*>(p);
    luaL_unref(e->L, LUA_REGISTRYINDEX, e->ref);
    return true;
}

Schedule::Entry gen_entry(lua_State *L, Schedule::Flag flags, int idx) {
    NNGN_LOG_CONTEXT_F();
    lua_pushvalue(L, idx);
    int ref = luaL_ref(L, LUA_REGISTRYINDEX);
    std::vector<std::byte> data(sizeof(entry));
    new (data.data()) entry{L, ref};
    return {call, destroy, data, flags};
}

auto next(
    sol::this_state sol, Schedule &s, Schedule::Flag flags,
    const sol::stack_function &f
) {
    return s.next(gen_entry(sol, flags, f.stack_index()));
}

auto in_ms(
    sol::this_state sol, Schedule &s, Schedule::Flag flags,
    std::chrono::milliseconds::rep r, const sol::stack_function &f
) {
    return s.in(
        std::chrono::milliseconds(r),
        gen_entry(sol, flags, f.stack_index()));
}

auto frame(
    sol::this_state sol, Schedule &s, Schedule::Flag flags,
    nngn::u64 frame, const sol::stack_function &f
) {
    s.frame(frame, gen_entry(sol, flags, f.stack_index()));
}

auto atexit_(sol::this_state sol, Schedule &s, const sol::stack_function &f)
    { return s.atexit(gen_entry(sol, {}, f.stack_index())); }

}

NNGN_LUA_PROXY(Schedule,
    sol::no_constructor,
    "HEARTBEAT", sol::var(Schedule::Flag::HEARTBEAT),
    "next", next,
    "in_ms", in_ms,
    "frame", frame,
    "atexit", atexit_,
    "cancel", &Schedule::cancel)
