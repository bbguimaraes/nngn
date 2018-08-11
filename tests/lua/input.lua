dofile "src/lua/path.lua"
local camera = require "nngn.lib.camera"
local common = require "tests/lua/common"
local input = require "nngn.lib.input"
local math = require "nngn.lib.math"
local player = require "nngn.lib.player"
local timing = require "nngn.lib.timing"
local utils = require "nngn.lib.utils"

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

local function test_input_b()
    local key = string.byte("B")
    common.assert_eq(nngn.renderers:debug(), 0)
    nngn.input:key_callback(key, Input.SEL_PRESS, Input.MOD_CTRL)
    common.assert_eq(nngn.renderers:debug(), 1)
    nngn.input:key_callback(
        key, Input.SEL_PRESS, Input.MOD_CTRL | Input.MOD_SHIFT)
    common.assert_eq(nngn.renderers:debug(), 0)
end

local function test_input_c()
    local key = string.byte("C")
    local c = nngn.camera
    nngn.camera:set_pos(1, 2, 3)
    nngn.input:key_callback(key, Input.KEY_PRESS, Input.MOD_CTRL)
    common.assert_eq({c:eye()}, {0, 0, c:fov_z() / c:zoom()}, common.deep_cmp)
    nngn.input:key_callback(
        key, Input.KEY_PRESS, Input.MOD_CTRL | Input.MOD_ALT)
    assert(c:perspective())
    nngn.input:key_callback(
        key, Input.KEY_PRESS, Input.MOD_CTRL | Input.MOD_ALT)
    assert(not c:perspective())
end

local function test_input_c_follow()
    local key = string.byte("C")
    nngn.input:key_callback(key, Input.KEY_PRESS, 0)
    common.assert_eq(camera.following(), nil)
    local e = nngn.entities:add()
    local p = player.add(e)
    camera.set_follow(nil)
    nngn.input:key_callback(key, Input.KEY_PRESS, 0)
    common.assert_eq(deref(camera.following()), deref(e))
    nngn.input:key_callback(key, Input.KEY_PRESS, 0)
    common.assert_eq(camera.following(), nil)
    player.remove(p)
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

local function test_input_p_tab()
    local key_p = string.byte("P")
    nngn.input:key_callback(key_p, Input.KEY_PRESS, Input.MOD_CTRL)
    common.assert_eq(player.n(), 1)
    local e0 = player.entity()
    common.assert_eq(deref(camera.following()), deref(e0))
    nngn.input:key_callback(key_p, Input.KEY_PRESS, Input.MOD_CTRL)
    common.assert_eq(player.n(), 2)
    common.assert_eq(deref(camera.following()), deref(e0))
    nngn.input:key_callback(key_p, Input.KEY_PRESS, Input.MOD_CTRL)
    common.assert_eq(player.n(), 3)
    common.assert_eq(deref(camera.following()), deref(e0))
    common.assert_eq(player.idx(), 0)
    nngn.input:key_callback(Input.KEY_TAB, Input.KEY_PRESS, 0)
    common.assert_eq(player.idx(), 1)
    nngn.input:key_callback(Input.KEY_TAB, Input.KEY_PRESS, 0)
    common.assert_eq(player.idx(), 2)
    nngn.input:key_callback(Input.KEY_TAB, Input.KEY_PRESS, 0)
    common.assert_eq(player.idx(), 0)
    nngn.input:key_callback(
        key_p, Input.KEY_PRESS, Input.MOD_CTRL | Input.MOD_SHIFT)
    common.assert_eq(player.n(), 2)
    common.assert_eq(player.idx(), 0)
    nngn.input:key_callback(
        key_p, Input.KEY_PRESS, Input.MOD_CTRL | Input.MOD_SHIFT)
    nngn.input:key_callback(
        key_p, Input.KEY_PRESS, Input.MOD_CTRL | Input.MOD_SHIFT)
    common.assert_eq(player.n(), 0)
end

local function test_input_v()
    local scale = nngn.timing:scale()
    local t = timing.scales()
    local key = string.byte("V")
    nngn.timing:set_scale(t[1])
    nngn.input:key_callback(key, Input.KEY_PRESS, Input.MOD_CTRL)
    common.assert_eq(nngn.timing:scale(), t[2], math.float_eq)
    nngn.input:key_callback(
        key, Input.KEY_PRESS, Input.MOD_CTRL | Input.MOD_SHIFT)
    common.assert_eq(nngn.timing:scale(), t[1], math.float_eq)
    nngn.timing:set_scale(scale)
end

local opengl = nngn:set_graphics(
    Graphics.OPENGL_ES_BACKEND,
    Graphics.opengl_params{maj = 3, min = 1, hidden = true})
if not opengl then nngn:set_graphics(Graphics.PSEUDOGRAPH) end
input.install()
nngn.entities:set_max(3)
nngn.graphics:resize_textures(2)
nngn.textures:set_max(2)
nngn.renderers:set_max_sprites(3)
nngn.animations:set_max(3)
player.set("src/lson/crono.lua")
common.setup_hook(1)
if opengl then test_input_i() end
test_input_b()
test_input_c()
test_input_c_follow()
test_input_p()
test_input_p_tab()
test_input_v()
nngn:exit()
