#ifndef NNGN_INPUT_GROUP_H
#define NNGN_INPUT_GROUP_H

#include <unordered_map>

#include "input.h"

namespace nngn {

class BindingGroup {
    struct Info { int ref = {}; Input::Selector selector = {}; };
    std::unordered_map<int, Info> m = {};
public:
    Info for_key(int key) const;
    Info for_event(int key, Input::Action action, Input::Modifier mods) const;
    bool add(lua_State *L, int key, Input::Selector s);
    bool remove(lua_State *L, int key);
};

}

#endif
