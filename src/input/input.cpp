#include "input.h"

#include <algorithm>
#include <utility>

#include <lua.hpp>

#include "lua/state.h"
#include "lua/utils.h"
#include "utils/log.h"

#include "group.h"

namespace nngn {

Input::Source::~Source() = default;
void Input::Source::get_keys(std::size_t n, std::int32_t *keys) const
    { std::fill_n(keys, n, 0); }

void Input::get_keys(std::size_t n, std::int32_t *keys) const {
    for(auto *const e = keys + n; keys != e; ++keys)
        if(this->overrides[static_cast<std::size_t>(*keys)])
            *keys = 1;
        else
            std::any_of(
                begin(this->sources), end(this->sources),
                [keys, key = std::exchange(*keys, 0)](auto &x) {
                    *keys = key;
                    x->get_keys(1, keys);
                    return *keys;
                });
}

void Input::add_source(std::unique_ptr<Source> p)
    { this->sources.emplace_back(std::move(p)); }

bool Input::remove_source(Source *p) {
    NNGN_LOG_CONTEXT_CF(Input);
    auto &v = this->sources;
    const auto i = std::find_if(
        begin(v), end(v), [p](auto &x) { return x.get() == p; });
    if(i == end(v)) {
        Log::l() << "source not found\n";
        return false;
    }
    v.erase(i);
    return true;
}

void Input::has_override(std::size_t n, std::int32_t *keys) const {
    std::transform(
        keys, keys + n, keys,
        [&o = this->overrides](auto k)
            { return o[static_cast<std::size_t>(k)]; });
}

bool Input::override_keys(
    bool pressed, std::size_t n, const std::int32_t *keys
) {
    const auto add = [&o = this->overrides](auto k) {
        auto r = o[static_cast<std::size_t>(k)];
        return r
            ? (Log::l() << "key " << k << " already overridden\n", false)
            : (r = true);
    };
    const auto remove = [&o = this->overrides](auto k) {
        auto r = o[static_cast<std::size_t>(k)];
        return r
            ? !(r = false)
            : (Log::l() << "override for key " << k << " not found\n", false);
    };
    return pressed
        ? std::all_of(keys, keys + n, add)
        : std::all_of(keys, keys + n, remove);
}

bool Input::register_callback() {
    NNGN_LOG_CONTEXT_CF(Input);
    if(this->callback_ref) {
        Log::l() << "already registered, ignoring\n";
        return false;
    }
    lua_pushvalue(this->L, -1);
    this->callback_ref = luaL_ref(this->L, LUA_REGISTRYINDEX);
    return true;
}

bool Input::remove_callback() {
    NNGN_LOG_CONTEXT_CF(Input);
    if(!this->callback_ref) {
        Log::l() << "no callback registered\n";
        return false;
    }
    luaL_unref(this->L, LUA_REGISTRYINDEX, this->callback_ref);
    this->callback_ref = {};
    return true;
}

bool Input::update() {
    return std::all_of(
        begin(this->sources), end(this->sources),
        [this](const auto &x) { return x->update(this); });
}

bool Input::key_callback(int key, Action action, Modifier mods) const {
    NNGN_LOG_CONTEXT_CF(Input);
    const bool press = action == Input::KEY_PRESS;
    if(!press && action != Input::KEY_RELEASE)
        return true;
    const auto call = [L = this->L, key, press, mods](int ref) {
        lua_pushcfunction(L, nngn::lua::msgh);
        lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
        lua_pushinteger(L, key);
        lua_pushboolean(L, press);
        lua_pushinteger(L, mods);
        NNGN_ANON_DECL(nngn::lua::defer_pop(L, 2));
        return lua_pcall(L, 3, 1, -5) == LUA_OK
            ? std::tuple(static_cast<bool>(lua_toboolean(L, -1)), true)
            : std::tuple(false, false);
    };
    if(this->callback_ref) {
        NNGN_LOG_CONTEXT("callback");
        const auto [ret, ok] = call(this->callback_ref);
        if(!ok)
            return false;
        if(ret)
            return true;
    }
    NNGN_LOG_CONTEXT("binding");
    if(auto g = this->m_binding_group) {
        if(const auto ref = g->for_event(key, action, mods).ref) {
            const auto [_, ok] = call(ref);
            return ok;
        }
    }
    return true;
}

}
