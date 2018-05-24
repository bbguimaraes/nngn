dofile "src/lua/path.lua"
local common = require "tests/lua/common"
local input = require "nngn.lib.input"

local function test_input_p()
    local function group() return deref(nngn.input:binding_group()) end
    local key = string.byte("P")
    common.assert_eq(group(), deref(input.input))
    nngn.input:key_callback(key, Input.KEY_PRESS, 0)
    common.assert_eq(group(), deref(input.paused_input))
    nngn.input:key_callback(key, Input.KEY_PRESS, 0)
    common.assert_eq(group(), deref(input.input))
end

input.install()
common.setup_hook(1)
test_input_p()
nngn:exit()
