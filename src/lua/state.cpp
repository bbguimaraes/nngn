#include "alloc.h"

#include "state.h"

#include "utils/log.h"

#include "utils.h"

namespace {

bool pcall(lua_State *L, auto &&f) {
    NNGN_ANON_DECL(nngn::lua::stack_mark(L));
    lua_pushcfunction(L, nngn::lua::msgh);
    auto pop = nngn::lua::defer_pop{L};
    if(!FWD(f)())
        return false;
    if(lua_pcall(L, 0, 0, -2) == LUA_OK)
        return true;
    pop.set_n(2);
    return false;
}

[[maybe_unused]] void check_alloc_info(const nngn::lua::alloc_info &info) {
    NNGN_LOG_CONTEXT_F();
    for(std::size_t i = 0; i != std::tuple_size_v<decltype(info.v)>; ++i) {
        if(info.v[i].n)
            nngn::Log::l()
                << "i[" << i << "].n == " << info.v[i].n << '\n';
        if(info.v[i].bytes)
            nngn::Log::l()
                << "i[" << i << "].bytes == " << info.v[i].bytes << '\n';
    }
}

}

namespace nngn::lua {

bool state_view::init(alloc_info *i) {
    NNGN_LOG_CONTEXT_CF(lua::state_view);
    this->destroy();
    if constexpr(Platform::lua_use_alloc)
        this->L = i ? lua_newstate(alloc_info::lua_alloc, i) : luaL_newstate();
    else {
        if(i)
            Log::l() << "compiled without custom Lua allocator,"
                " ignoring `alloc_info` parameter\n";
        this->L = luaL_newstate();
    }
    if(!this->L)
        return Log::l() << "failed\n", false;
    luaL_openlibs(this->L);
    return true;
}

void state_view::destroy(void) {
    NNGN_LOG_CONTEXT_CF(lua::state_view);
    if(!this->L)
        return;
    [[maybe_unused]] const auto *const a = this->allocator().second;
    lua_close(std::exchange(this->L, nullptr));
    if constexpr(Platform::debug)
        if(a)
            check_alloc_info(*static_cast<const alloc_info*>(a));
}

bool state_view::doarg(std::string_view s) const {
    return s[0] == '@' ? this->dofile(s.substr(1)) : this->dostring(s);
}

bool state_view::dofile(std::string_view f) const {
    NNGN_LOG_CONTEXT_CF(lua::state_view);
    NNGN_LOG_CONTEXT(f.data());
    return ::pcall(this->L, [this, f] {
        if(luaL_loadfile(this->L, f.data()) == LUA_OK)
            return true;
        Log::l() << "failed to load file: " << lua_tostring(L, -1) << '\n';
        lua_pop(L, 1);
        return false;
    });
}

bool state_view::dostring(std::string_view src) const {
    NNGN_LOG_CONTEXT_CF(lua::state_view);
    NNGN_LOG_CONTEXT(src.data());
    return ::pcall(this->L, [this, src] {
        if(luaL_loadstring(this->L, src.data()) == LUA_OK)
            return true;
        Log::l() << "failed to load string: " << lua_tostring(L, -1) << '\n';
        lua_pop(L, 1);
        return false;
    });
}

bool state_view::dofunction(std::string_view f) const {
    NNGN_LOG_CONTEXT_CF(lua::state_view);
    NNGN_LOG_CONTEXT(f.data());
    return ::pcall(this->L, [this, f] {
        if(lua_getglobal(this->L, f.data()) == LUA_TFUNCTION)
            return true;
        Log::l() << "failed to find function: " << f.data() << '\n';
        return false;
    });
}

bool state_view::heartbeat() const {
    NNGN_LOG_CONTEXT_CF(lua::state_view);
    NNGN_ANON_DECL(stack_mark(this->L));
    auto pop = defer_pop{this->L};
    lua_pushcfunction(this->L, nngn::lua::msgh);
    if(lua_getglobal(this->L, "heartbeat") != LUA_TFUNCTION)
        return true;
    pop.set_n(2);
    if(lua_pcall(this->L, 0, 0, 1) == LUA_OK)
        return true;
    Log::l() << lua_tostring(this->L, -1) << '\n';
    return false;
}

}
