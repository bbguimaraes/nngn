#include "lua/function.h"
#include "lua/register.h"
#include "lua/table.h"

#include "mouse.h"

using nngn::MouseInput;

namespace {

bool move_callback(MouseInput &i, nngn::lua::table_view pos) {
    return i.move_callback({pos[1], pos[2]});
}

void register_mouse(nngn::lua::table_view t) {
    t["register_button_callback"] = &MouseInput::register_button_callback;
    t["register_move_callback"] = &MouseInput::register_move_callback;
    t["button_callback"] = &MouseInput::button_callback;
    t["move_callback"] = move_callback;
}

}

NNGN_LUA_DECLARE_USER_TYPE(MouseInput)
NNGN_LUA_PROXY(MouseInput, register_mouse)
