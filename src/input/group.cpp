#include "group.h"

#include <iomanip>

#include <lua.hpp>

#include "utils/log.h"

namespace {

template<auto mask>
bool set_bits_match(int check, int value) { return !(~check & value & mask); }

}

namespace nngn {

auto BindingGroup::for_key(int key) const -> Info {
    if(const auto i = this->m.find(key); i != this->m.cend())
        return i->second;
    return {};
}

auto BindingGroup::for_event(
    int key, Input::Action action, Input::Modifier mods
) const -> Info {
    const auto matches = [action, mods](auto s) {
        constexpr auto act_mask = Input::Selector::PRESS;
        constexpr auto mod_mask = Input::Selector::CTRL | Input::Selector::ALT;
        return set_bits_match<act_mask>(action, s)
            && set_bits_match<mod_mask>(mods, s);
    };
    const auto i = this->m.find(key);
    Info ret = {};
    if(i != this->m.cend() && matches(i->second.selector))
        ret = i->second;
    return ret;
}

bool BindingGroup::add(lua_State *L, int key, Input::Selector s) {
    auto i = this->m.find(key);
    if(i != this->m.end()) {
        const auto key_c = static_cast<char>(key);
        Log::l()
            << "ignoring callback for key " << key << " ("
            << std::quoted(std::string_view{&key_c, 1})
            << "), already registered\n";
        return false;
    }
    i = this->m.insert(i, {key, {}});
    lua_pushvalue(L, -1);
    i->second = {luaL_ref(L, LUA_REGISTRYINDEX), s};
    return true;
}

bool BindingGroup::remove(lua_State *L, int key) {
    const auto i = this->m.find(key);
    if(i == this->m.end()) {
        const auto key_c = static_cast<char>(key);
        Log::l()
            << "binding for key " << key << " ("
            << std::quoted(std::string_view{&key_c, 1})
            << ") does not exist\n";
        return false;
    }
    luaL_unref(L, LUA_REGISTRYINDEX, i->second.ref);
    this->m.erase(i);
    return true;
}

}
