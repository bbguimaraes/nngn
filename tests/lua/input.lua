dofile "src/lua/path.lua"
local camera = require "nngn.lib.camera"
local common = require "tests/lua/common"
local entity = require "nngn.lib.entity"
local input = require "nngn.lib.input"
local math = require "nngn.lib.math"
local player = require "nngn.lib.player"
local textbox = require "nngn.lib.textbox"
local timing = require "nngn.lib.timing"
local utils = require "nngn.lib.utils"

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

local function test_input_c()
    local key <const>, ctrl <const> = string.byte("C"), Input.MOD_CTRL
    local c = nngn:camera()
    c:set_pos(1, 2, 3)
    nngn:input():key_callback(key, Input.KEY_PRESS, ctrl)
    common.assert_eq({c:pos()}, {0, 0, c:z_for_fov()}, common.deep_cmp)
    nngn:input():key_callback(key, Input.KEY_PRESS, ctrl | Input.MOD_ALT)
    assert(c:perspective())
    nngn:input():key_callback(key, Input.KEY_PRESS, ctrl | Input.MOD_ALT)
    assert(not c:perspective())
end

local function test_input_c_follow()
    local key = string.byte("C")
    nngn:input():key_callback(key, Input.KEY_PRESS, 0)
    common.assert_eq(camera.following(), nil)
    local e = nngn:entities():add()
    local p = player.add(e)
    camera.set_follow(nil)
    nngn:input():key_callback(key, Input.KEY_PRESS, 0)
    assert(camera.following())
    common.assert_eq(deref(camera.following()), deref(e))
    nngn:input():key_callback(key, Input.KEY_PRESS, 0)
    common.assert_eq(camera.following(), nil)
    player.remove(p)
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

local function test_input_p_tab()
    local key_p = string.byte("P")
    nngn:input():key_callback(key_p, Input.KEY_PRESS, Input.MOD_CTRL)
    common.assert_eq(player.n(), 1)
    local e0 = player.entity()
    common.assert_eq(deref(camera.following()), deref(e0))
    nngn:input():key_callback(key_p, Input.KEY_PRESS, Input.MOD_CTRL)
    common.assert_eq(player.n(), 2)
    common.assert_eq(deref(camera.following()), deref(e0))
    nngn:input():key_callback(key_p, Input.KEY_PRESS, Input.MOD_CTRL)
    common.assert_eq(player.n(), 3)
    common.assert_eq(deref(camera.following()), deref(e0))
    common.assert_eq(player.idx(), 0)
    local r0 = e0:renderer()
    local r1 = player.get(1).entity:renderer()
    local r2 = player.get(2).entity:renderer()
    local cmp = function(t)
        common.assert_eq({
            nngn:renderers():selected(r0),
            nngn:renderers():selected(r1),
            nngn:renderers():selected(r2),
        }, t, common.deep_cmp)
    end
    cmp{false, false, false}
    nngn:input():key_callback(Input.KEY_TAB, Input.KEY_PRESS, 0)
    common.assert_eq(player.idx(), 1)
    cmp{false, true, false}
    nngn:input():key_callback(Input.KEY_TAB, Input.KEY_RELEASE, 0)
    cmp{false, false, false}
    nngn:input():key_callback(Input.KEY_TAB, Input.KEY_PRESS, 0)
    common.assert_eq(player.idx(), 2)
    cmp{false, false, true}
    nngn:input():key_callback(Input.KEY_TAB, Input.KEY_RELEASE, 0)
    cmp{false, false, false}
    nngn:input():key_callback(Input.KEY_TAB, Input.KEY_PRESS, 0)
    common.assert_eq(player.idx(), 0)
    cmp{true, false, false}
    nngn:input():key_callback(
        key_p, Input.KEY_PRESS, Input.MOD_CTRL | Input.MOD_SHIFT)
    common.assert_eq(player.n(), 2)
    common.assert_eq(player.idx(), 0)
    nngn:input():key_callback(
        key_p, Input.KEY_PRESS, Input.MOD_CTRL | Input.MOD_SHIFT)
    nngn:input():key_callback(
        key_p, Input.KEY_PRESS, Input.MOD_CTRL | Input.MOD_SHIFT)
    common.assert_eq(player.n(), 0)
end

local function test_input_p_alt()
    local key = string.byte("P")
    nngn:input():key_callback(key, Input.KEY_PRESS, Input.MOD_CTRL)
    local e = player.entity()
    common.assert_eq(nngn:entities():name(e), "crono")
    nngn:input():key_callback(
        key, Input.KEY_PRESS, Input.MOD_CTRL | Input.MOD_ALT)
    common.assert_eq(nngn:entities():name(e), "link")
    nngn:input():key_callback(
        key, Input.KEY_PRESS, Input.MOD_CTRL | Input.MOD_ALT)
    common.assert_eq(nngn:entities():name(e), "link_sh")
    nngn:input():key_callback(
        key, Input.KEY_PRESS, Input.MOD_CTRL | Input.MOD_ALT | Input.MOD_SHIFT)
    common.assert_eq(nngn:entities():name(e), "link")
    player.remove(player.get(0))
end

local function test_input_v()
    local scale = nngn:timing():scale()
    local t = timing.scales()
    local key = string.byte("V")
    nngn:timing():set_scale(t[1])
    nngn:input():key_callback(key, Input.KEY_PRESS, Input.MOD_CTRL)
    common.assert_eq(nngn:timing():scale(), t[2], math.float_eq)
    nngn:input():key_callback(
        key, Input.KEY_PRESS, Input.MOD_CTRL | Input.MOD_SHIFT)
    common.assert_eq(nngn:timing():scale(), t[1], math.float_eq)
    nngn:timing():set_scale(scale)
end

local function test_input_space()
    local key = string.byte(" ")
    textbox.set("title", "text")
    common.assert_eq(nngn:textbox():speed(), textbox.SPEED_NORMAL)
    nngn:input():key_callback(key, Input.KEY_PRESS, 0)
    common.assert_eq(nngn:textbox():speed(), textbox.SPEED_FAST)
    nngn:input():key_callback(key, Input.KEY_RELEASE, 0)
    common.assert_eq(nngn:textbox():speed(), textbox.SPEED_NORMAL)
    nngn:textbox():set_cur(4)
    nngn:input():key_callback(key, Input.KEY_PRESS, 0)
    common.assert_eq(nngn:textbox():title(), "")
    common.assert_eq(nngn:textbox():text(), "")
end

local function test_input_g()
    nngn:grid():set_enabled(false)
    local key = string.byte("G")
    nngn:input():key_callback(key, Input.KEY_PRESS, Input.MOD_CTRL)
    assert(nngn:grid():enabled())
    nngn:input():key_callback(key, Input.KEY_PRESS, Input.MOD_CTRL)
    assert(not nngn:grid():enabled())
end

local function test_input_o()
    local key = string.byte("O")
    nngn:colliders():set_resolve(true)
    nngn:input():key_callback(key, Input.KEY_PRESS, Input.MOD_CTRL)
    assert(not nngn:colliders():resolve())
    nngn:input():key_callback(key, Input.KEY_PRESS, Input.MOD_CTRL)
    assert(nngn:colliders():resolve())
end

local function test_input_n()
    local key = string.byte("N")
    local cmp = function(t)
        common.assert_eq({nngn:lighting():ambient_light()}, t, common.deep_cmp)
    end
    nngn:lighting():set_ambient_light(1, 1, 1, 1)
    nngn:input():key_callback(key, Input.KEY_PRESS, Input.MOD_CTRL)
    cmp{1, 1, 1, 1}
    nngn:input():key_callback(
        key, Input.KEY_PRESS, Input.MOD_CTRL | Input.MOD_SHIFT)
    cmp{0.5, 0.5, 0.5, 1}
    nngn:input():key_callback(
        key, Input.KEY_PRESS, Input.MOD_CTRL | Input.MOD_SHIFT)
    cmp{0.25, 0.25, 0.25, 1}
    nngn:input():key_callback(key, Input.KEY_PRESS, Input.MOD_CTRL)
    cmp{0.5, 0.5, 0.5, 1}
    nngn:input():key_callback(key, Input.KEY_PRESS, Input.MOD_CTRL)
    cmp{1, 1, 1, 1}
end

local function test_input_n_alt()
    local key = string.byte("N")
    nngn:input():key_callback(
        key, Input.KEY_PRESS, Input.MOD_CTRL | Input.MOD_ALT)
    assert(nngn:lighting():sun_light())
    nngn:input():key_callback(
        key, Input.KEY_PRESS, Input.MOD_CTRL | Input.MOD_ALT)
    assert(not nngn:lighting():sun_light())
end

local function test_input_l()
    local key = string.byte("L")
    nngn:lighting():set_enabled(false)
    nngn:input():key_callback(key, Input.KEY_PRESS, Input.MOD_CTRL)
    assert(nngn:lighting():enabled())
    nngn:input():key_callback(key, Input.KEY_PRESS, Input.MOD_CTRL)
    assert(not nngn:lighting():enabled())
end

local function test_input_z()
    assert(not nngn:renderers():zsprites())
    assert(not nngn:lighting():zsprites())
    local key = string.byte("Z")
    nngn:input():key_callback(key, Input.KEY_PRESS, Input.MOD_CTRL)
    assert(nngn:renderers():zsprites())
    assert(nngn:lighting():zsprites())
    nngn:input():key_callback(key, Input.KEY_PRESS, Input.MOD_CTRL)
    assert(not nngn:renderers():zsprites())
    assert(not nngn:lighting():zsprites())
end

local function test_input_s_control()
    assert(not nngn:lighting():shadows_enabled())
    local key = string.byte("S")
    nngn:input():key_callback(key, Input.KEY_PRESS, Input.MOD_CTRL)
    assert(nngn:lighting():shadows_enabled())
    nngn:input():key_callback(key, Input.KEY_PRESS, Input.MOD_CTRL)
    assert(not nngn:lighting():shadows_enabled())
end

local function test_input_m()
    local key = string.byte("M")
    nngn:map():set_enabled(true)
    nngn:input():key_callback(key, Input.KEY_PRESS, Input.MOD_CTRL)
    assert(not nngn:map():enabled())
    nngn:input():key_callback(key, Input.KEY_PRESS, Input.MOD_CTRL)
    assert(nngn:map():enabled())
end

local function test_menu_fairy()
    local key_e <const>, key_f <const> = string.byte("EF", 1, -1)
    local p = player.add()
    assert(not p.data.fairy)
    nngn:input():key_callback(key_e, Input.KEY_PRESS, 0)
    nngn:input():key_callback(key_e, Input.KEY_PRESS, 0)
    nngn:input():key_callback(key_e, Input.KEY_PRESS, 0)
    nngn:input():key_callback(key_f, Input.KEY_PRESS, 0)
    nngn:input():key_callback(key_f, Input.KEY_PRESS, 0)
    assert(p.data.fairy)
    nngn:input():key_callback(key_f, Input.KEY_PRESS, 0)
    assert(not p.data.fairy)
    player.remove(p)
end

local function test_menu_light()
    local key <const> = string.byte("F")
    local p = player.add(entity.load(nil, "src/lson/crono.lua"))
    assert(not p.data.light)
    nngn:input():key_callback(key, Input.KEY_PRESS, 0)
    assert(p.data.light)
    nngn:input():key_callback(key, Input.KEY_PRESS, 0)
    assert(not p.data.light)
    player.remove(p)
end

local function test_menu_flashlight()
    local key_e <const>, key_f <const> = string.byte("EF", 1, -1)
    local p = player.add(entity.load(nil, "src/lson/crono.lua"))
    assert(not p.data.flashlight)
    nngn:input():key_callback(key_e, Input.KEY_PRESS, 0)
    nngn:input():key_callback(key_e, Input.KEY_PRESS, 0)
    nngn:input():key_callback(key_f, Input.KEY_PRESS, 0)
    nngn:input():key_callback(key_f, Input.KEY_PRESS, 0)
    assert(p.data.flashlight)
    nngn:input():key_callback(key_f, Input.KEY_PRESS, 0)
    assert(not p.data.flashlight)
    player.remove(p)
end

local opengl = nngn:set_graphics(
    Graphics.OPENGL_ES_BACKEND,
    Graphics.opengl_params{maj = 3, min = 1, hidden = true})
if not opengl then nngn:set_graphics(Graphics.PSEUDOGRAPH) end
input.install()
nngn:entities():set_max(7)
nngn:graphics():resize_textures(4)
nngn:textures():set_max(4)
nngn:renderers():set_max_sprites(6)
nngn:renderers():set_max_screen_sprites(6)
nngn:animations():set_max(3)
nngn:colliders():set_max_colliders(4)
player.set{
    "src/lson/crono.lua",
    "src/lson/zelda/link.lua",
    "src/lson/zelda/link_sh.lua",
}
common.setup_hook(1)
test_get_keys()
if opengl then test_input_i() end
test_input_b()
test_input_c()
test_input_c_follow()
test_input_p()
test_input_p_tab()
test_input_p_alt()
test_input_v()
test_input_space()
test_input_g()
test_input_o()
test_input_l()
test_input_n()
test_input_n_alt()
test_input_z()
test_input_s_control()
test_input_m()
test_menu_fairy()
test_menu_light()
test_menu_flashlight()
nngn:exit()
