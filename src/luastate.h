#ifndef NNGN_LUASTATE_H
#define NNGN_LUASTATE_H

#include <string>
#include <vector>

#include "utils/scoped.h"

struct lua_State;

struct LuaStack {
    static void pop(lua_State *L, int n);
    static auto guard(lua_State *L, int pop) {
        return nngn::make_scoped(nngn::delegate_fn<LuaStack::pop>{}, L, pop);
    }
};

class LuaState {
    using register_f = void (*)(lua_State*);
    static std::vector<register_f> &get_registry();
public:
    class Traceback {
        lua_State *L = nullptr;
    public:
        explicit constexpr Traceback(lua_State *l) noexcept : L(l) {}
        std::string str() const;
        friend std::ostream &operator<<(
            std::ostream &os, const LuaState::Traceback &t);
    };
    struct Register { explicit Register(register_f f); };
    lua_State *L = nullptr;
    static int msgh(lua_State *L);
    static void print_stack(lua_State *L);
    static void print_traceback(lua_State *L);
    LuaState() = default;
    explicit LuaState(lua_State *l) : L(l) {}
    LuaState(const LuaState &) = delete;
    LuaState(const LuaState &&) = delete;
    LuaState &operator=(const LuaState &) = delete;
    LuaState &operator=(const LuaState &&) = delete;
    ~LuaState() { this->destroy(); }
    bool init();
    void destroy();
    void register_all() const;
    void log_traceback() const;
    bool dofile(std::string_view filename) const;
    bool dostring(std::string_view src) const;
    bool dofunction(std::string_view name) const;
    bool heartbeat() const;
};

inline LuaState::Register::Register(register_f f)
    { LuaState::get_registry().push_back(f); }

#define NNGN_LUA_PROXY(T, ...) \
    static auto NNGN_LUA_REGISTER_##T##_F = [](lua_State *L) \
        { sol::state_view(L).new_usertype<T>(#T, __VA_ARGS__); }; \
    static LuaState::Register NNGN_LUA_REGISTER_##T(NNGN_LUA_REGISTER_##T##_F);

std::ostream &operator<<(std::ostream &os, const LuaState::Traceback &t);

#endif
