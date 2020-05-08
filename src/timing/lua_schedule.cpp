#include "lua/function.h"
#include "lua/register.h"
#include "lua/table.h"

#include "utils/def.h"
#include "utils/delegate.h"
#include "utils/log.h"
#include "utils/scoped.h"

#include "schedule.h"

using nngn::Schedule;

namespace {

struct entry {
    lua_State *L = nullptr;
    int ref = 0;
};

bool entry_call(void *p) {
    NNGN_LOG_CONTEXT("lua task");
    const auto *e = static_cast<const entry*>(p);
    const nngn::lua::state_view lua = {e->L};
    const int msgh = lua.push(nngn::lua::msgh).index();
    lua_rawgeti(e->L, LUA_REGISTRYINDEX, e->ref);
    const auto pop = nngn::lua::defer_pop(lua);
    return lua_pcall(lua, 0, 0, msgh) == LUA_OK;
}

bool entry_destroy(void *p) {
    NNGN_LOG_CONTEXT_F();
    const auto *e = static_cast<const entry*>(p);
    luaL_unref(e->L, LUA_REGISTRYINDEX, e->ref);
    return true;
}

Schedule::Entry gen_entry(nngn::lua::state_view lua, Schedule::Flag f, int i) {
    NNGN_LOG_CONTEXT_F();
    lua_pushvalue(lua, i);
    const int ref = luaL_ref(lua, LUA_REGISTRYINDEX);
    std::vector<std::byte> data(sizeof(entry));
    new (data.data()) entry{lua, ref};
    return {entry_call, entry_destroy, data, f};
}

auto next(
    Schedule &s, Schedule::Flag f, nngn::lua::function_view fn,
    nngn::lua::state_view lua)
{
    return nngn::narrow<lua_Integer>(s.next(gen_entry(lua, f, fn.index())));
}

auto in_ms(
    Schedule &s, Schedule::Flag f, std::chrono::milliseconds::rep r,
    nngn::lua::function_view fn, nngn::lua::state_view lua)
{
    return nngn::narrow<lua_Integer>(
        s.in(std::chrono::milliseconds{r}, gen_entry(lua, f, fn.index())));
}

auto frame(
    Schedule &s, Schedule::Flag f, lua_Integer frame,
    nngn::lua::function_view fn, nngn::lua::state_view lua)
{
    return nngn::narrow<lua_Integer>(
        s.frame(nngn::narrow<nngn::u64>(frame), gen_entry(lua, f, fn.index())));
}

auto atexit_(
    Schedule &s, nngn::lua::function_view f, nngn::lua::state_view lua)
{
    return nngn::narrow<lua_Integer>(s.atexit(gen_entry(lua, {}, f.index())));
}

bool cancel(Schedule &s, lua_Integer i) {
    return s.cancel(nngn::narrow<std::size_t>(i));
}

void register_schedule(nngn::lua::table_view t) {
    static constexpr nngn::to<lua_Integer> cast = {};
    t["IGNORE_FAILURES"] = Schedule::Flag::IGNORE_FAILURES;
    t["HEARTBEAT"] = Schedule::Flag::HEARTBEAT;
    t["n"] = [](const Schedule &s) { return cast(s.n()); };
    t["n_atexit"] = [](const Schedule &s) { return cast(s.n_atexit()); };
    t["next"] = next;
    t["in_ms"] = in_ms;
    t["frame"] = frame;
    t["atexit"] = atexit_;
    t["cancel"] = cancel;
}

}

NNGN_LUA_DECLARE_USER_TYPE(Schedule)
NNGN_LUA_PROXY(Schedule, register_schedule)
