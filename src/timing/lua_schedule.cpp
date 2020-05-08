#include "lua/state.h"

#include "utils/def.h"
#include "utils/delegate.h"
#include "utils/log.h"
#include "utils/scoped.h"

#include "schedule.h"

using nngn::Schedule;

namespace {

struct entry { lua_State *L = {}; int ref = {}; };

bool call(void *p) {
    NNGN_LOG_CONTEXT("lua task");
    const auto *e = static_cast<const entry*>(p);
    lua_pushcfunction(e->L, nngn::lua::msgh);
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
    Schedule &s, Schedule::Flag flags, nngn::lua::function f,
    nngn::lua::state_arg lua
) {
    return s.next(gen_entry(lua, flags, f.stack_index()));
}

auto in_ms(
    Schedule &s, Schedule::Flag flags, std::chrono::milliseconds::rep r,
    nngn::lua::function f, nngn::lua::state_arg lua
) {
    return s.in(
        std::chrono::milliseconds(r),
        gen_entry(lua, flags, f.stack_index()));
}

auto frame(
    Schedule &s, Schedule::Flag flags, nngn::u64 frame, nngn::lua::function f,
    nngn::lua::state_arg lua
) {
    s.frame(frame, gen_entry(lua, flags, f.stack_index()));
}

auto atexit_(Schedule &s, nngn::lua::function f, nngn::lua::state_arg lua) {
    return s.atexit(gen_entry(lua, {}, f.stack_index()));
}

}

using nngn::lua::var;

NNGN_LUA_PROXY(Schedule,
    "IGNORE_FAILURES", var(Schedule::Flag::IGNORE_FAILURES),
    "HEARTBEAT", var(Schedule::Flag::HEARTBEAT),
    "n", &Schedule::n,
    "n_atexit", &Schedule::n_atexit,
    "next", next,
    "in_ms", in_ms,
    "frame", frame,
    "atexit", atexit_,
    "cancel", &Schedule::cancel)
