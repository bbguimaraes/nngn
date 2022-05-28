#include <string>
#include "xxx_elysian_lua_frame.h"

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

int next(elysian::lua::ThreadView L) {
    const auto frame = LuaStackFrame<
        nngn::lua::sol_user_type<Schedule&>,
        lua_Integer,
        nngn::lua::sol_user_type<const sol::stack_function>>(L);
    auto [s, fl, f] = frame.values();
    return frame.return_values(s->next(
        gen_entry(L, static_cast<Schedule::Flag>(fl), f->stack_index())));
}

int in_ms(elysian::lua::ThreadView L) {
    const auto frame = LuaStackFrame<
        nngn::lua::sol_user_type<Schedule&>,
        lua_Integer,
        std::chrono::milliseconds::rep,
        nngn::lua::sol_user_type<const sol::stack_function>>(L);
    auto [s, fl, r, f] = frame.values();
    return frame.return_values(s->in(
        std::chrono::milliseconds(r),
        gen_entry(L, static_cast<Schedule::Flag>(fl), f->stack_index())));
}

int frame(elysian::lua::ThreadView L) {
    const auto frame = LuaStackFrame<
        nngn::lua::sol_user_type<Schedule&>,
        lua_Integer,
        nngn::u64,
        nngn::lua::sol_user_type<const sol::stack_function>>(L);
    auto [s, fl, fr, f] = frame.values();
    return frame.return_values(s->frame(
        /*XXX*/static_cast<nngn::u64>(fr),
        gen_entry(L, static_cast<Schedule::Flag>(fl), f->stack_index())));
}

int atexit_(elysian::lua::ThreadView L) {
    const auto frame = LuaStackFrame<
        nngn::lua::sol_user_type<Schedule&>,
        nngn::lua::sol_user_type<const sol::stack_function>>(L);
    auto [s, f] = frame.values();
    return frame.return_values(s->atexit(gen_entry(L, {}, f->stack_index())));
}

}

using nngn::lua::var;

NNGN_LUA_PROXY(Schedule,
    "IGNORE_FAILURES", var(Schedule::Flag::IGNORE_FAILURES),
    "HEARTBEAT", var(Schedule::Flag::HEARTBEAT),
    "n", &Schedule::n,
    "n_atexit", &Schedule::n_atexit,
    "next", elysian_lua_wrapper<next>,
    "in_ms", elysian_lua_wrapper<in_ms>,
    "frame", elysian_lua_wrapper<frame>,
    "atexit", elysian_lua_wrapper<atexit_>,
    "cancel", &Schedule::cancel)
