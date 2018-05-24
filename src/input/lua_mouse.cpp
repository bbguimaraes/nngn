#include "lua/state.h"

#include "mouse.h"

using nngn::MouseInput;

NNGN_LUA_PROXY(MouseInput,
    "register_button_callback", &MouseInput::register_button_callback,
    "register_move_callback", &MouseInput::register_move_callback,
    "button_callback", &MouseInput::button_callback,
    "move_callback", &MouseInput::move_callback)
