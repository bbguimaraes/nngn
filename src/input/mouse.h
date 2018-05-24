#ifndef NNGN_INPUT_MOUSE_H
#define NNGN_INPUT_MOUSE_H

#include "math/vec2.h"

struct lua_State;

namespace nngn {

/**
 * Mouse event manager.
 * Registered Lua callback functions are later called when events happen.
 * Events are manually triggered using the `*_callback(...)` functions.
 */
class MouseInput {
public:
    enum class Action : std::uint8_t { PRESS = 1, RELEASE = 0 };
    void init(lua_State *L_) { this->L = L_; }
    /**
     * Registers a Lua function called when a button is pressed.
     * The Lua stack is expected to contain the function object at the top.
     */
    void register_button_callback();
    /**
     * Registers a Lua function called when the mouse position changes.
     * The Lua stack is expected to contain the function object at the top.
     */
    void register_move_callback();
    /** Triggers a button event. */
    bool button_callback(int button, int action, int mods);
    /** Triggers a move event. */
    bool move_callback(dvec2 pos);
private:
    lua_State *L = {};
    int button_cb = {}, move_cb = {};
};

}

#endif
