dofile "src/lua/path.lua"
local common = require "tests/lua/common"
local input = require "nngn.lib.input"

local function test_get_keys()
    local i <const> = nngn:input()
    local keys <const> = {string.byte("A"), string.byte("B"), string.byte("C")}
    common.assert_eq(i:get_keys(keys), {0, 0, 0}, common.deep_cmp)
    assert(i:override_key(true, keys[1]))
    common.assert_eq(i:get_keys(keys), {1, 0, 0}, common.deep_cmp)
    assert(i:override_key(true, keys[3]))
    common.assert_eq(i:get_keys(keys), {1, 0, 1}, common.deep_cmp)
    assert(i:override_keys(false, {keys[1], keys[3]}))
    common.assert_eq(i:get_keys(keys), {0, 0, 0}, common.deep_cmp)
end

local function test_input_i()
    local key = string.byte("I")
    common.assert_eq(nngn:graphics():swap_interval(), 1)
    assert(nngn:input():key_callback(key, Input.KEY_PRESS, Input.MOD_CTRL))
    common.assert_eq(nngn:graphics():swap_interval(), 2)
    assert(nngn:input():key_callback(
        key, Input.KEY_PRESS, Input.MOD_CTRL | Input.MOD_SHIFT))
    common.assert_eq(nngn:graphics():swap_interval(), 1)
    assert(nngn:input():key_callback(
        key, Input.KEY_PRESS, Input.MOD_CTRL | Input.MOD_SHIFT))
    common.assert_eq(nngn:graphics():swap_interval(), 0)
end

local function test_input_b()
    local key = string.byte("B")
    common.assert_eq(nngn:renderers():debug(), 0)
    nngn:input():key_callback(key, Input.SEL_PRESS, Input.MOD_CTRL)
    common.assert_eq(nngn:renderers():debug(), 1)
    nngn:input():key_callback(
        key, Input.SEL_PRESS, Input.MOD_CTRL | Input.MOD_SHIFT)
    common.assert_eq(nngn:renderers():debug(), 0)
end

local function test_input_p()
    local function group() return deref(nngn:input():binding_group()) end
    local key = string.byte("P")
    common.assert_eq(group(), deref(input.input))
    nngn:input():key_callback(key, Input.KEY_PRESS, 0)
    common.assert_eq(group(), deref(input.paused_input))
    nngn:input():key_callback(key, Input.KEY_PRESS, 0)
    common.assert_eq(group(), deref(input.input))
end

local opengl = nngn:set_graphics(
    Graphics.OPENGL_ES_BACKEND,
    Graphics.opengl_params{maj = 3, min = 1, hidden = true})
if not opengl then nngn:set_graphics(Graphics.PSEUDOGRAPH) end
input.install()
common.setup_hook(1)
test_get_keys()
if opengl then test_input_i() end
test_input_b()
test_input_p()
nngn:exit()
