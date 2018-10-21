dofile "src/lua/path.lua"
local common = require "tests/lua/common"
local input = require "nngn.lib.input"

local function test_input_i()
    local key = string.byte("I")
    common.assert_eq(nngn.graphics:swap_interval(), 1)
    nngn.input:key_callback(
        key, Input.KEY_PRESS, Input.MOD_CTRL)
    common.assert_eq(nngn.graphics:swap_interval(), 2)
    nngn.input:key_callback(
        key, Input.KEY_PRESS, Input.MOD_CTRL | Input.MOD_SHIFT)
    common.assert_eq(nngn.graphics:swap_interval(), 1)
    nngn.input:key_callback(
        key, Input.KEY_PRESS, Input.MOD_CTRL | Input.MOD_SHIFT)
    common.assert_eq(nngn.graphics:swap_interval(), 0)
end

local function test_input_p()
    local function group() return deref(nngn.input:binding_group()) end
    local key = string.byte("P")
    common.assert_eq(group(), deref(input.input))
    nngn.input:key_callback(key, Input.KEY_PRESS, 0)
    common.assert_eq(group(), deref(input.paused_input))
    nngn.input:key_callback(key, Input.KEY_PRESS, 0)
    common.assert_eq(group(), deref(input.input))
end

local opengl = nngn:set_graphics(
    Graphics.OPENGL_ES_BACKEND,
    Graphics.opengl_params{maj = 3, min = 1, hidden = true})
if not opengl then nngn:set_graphics(Graphics.PSEUDOGRAPH) end
input.install()
common.setup_hook(1)
if opengl then test_input_i() end
test_input_p()
nngn:exit()
