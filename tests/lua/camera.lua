dofile "src/lua/path.lua"
local camera = require "nngn.lib.camera"
local common = require "tests/lua/common"
local nngn_math = require "nngn.lib.math"
local utils = require "nngn.lib.utils"

local function test_reset_common(c)
    common.assert_eq({c:vel()}, {0, 0, 0}, common.deep_cmp)
    common.assert_eq(c:max_vel(), camera.MAX_VEL)
    common.assert_eq(c:damp(), 5)
    common.assert_eq({c:acc()}, {0, 0, 0}, common.deep_cmp)
    common.assert_eq({c:rot_vel()}, {0, 0, 0}, common.deep_cmp)
    common.assert_eq({c:rot_acc()}, {0, 0, 0}, common.deep_cmp)
    common.assert_eq(c:zoom_vel(), 0)
    common.assert_eq(c:zoom_acc(), 0)
end

local function test_reset_ortho()
    local c = Camera.new()
    camera.set(c)
    camera.reset(1)
    test_reset_common(c)
    common.assert_eq({c:pos()}, {0, 0, 0}, common.deep_cmp)
    common.assert_eq({c:rot()}, {0, 0, 0}, common.deep_cmp)
    common.assert_eq(c:zoom(), 1)
end

local function test_reset_persp()
    local c = Camera.new()
    camera.set(c)
    local a, w, h, zoom = math.rad(60), 800, 600, 2
    local cmp = function(x, y)
        return nngn_math.float_eq(x, y, 32 * Math.FLOAT_EPSILON)
    end
    c:set_screen(w, h)
    c:set_perspective(true)
    local z = c:z_for_fov()
    camera.reset(zoom)
    test_reset_common(c)
    local pos = {c:pos()}
    common.assert_eq(pos[1], 0)
    common.assert_eq(pos[2], math.sin(a) * -z / zoom, cmp)
    common.assert_eq(pos[3], math.cos(a) *  z / zoom, cmp)
    local rot = {c:rot()}
    common.assert_eq(rot[1], a, nngn_math.float_eq)
    common.assert_eq(rot[2], 0)
    common.assert_eq(rot[3], 0)
    common.assert_eq(c:zoom(), 1)
end

local function test_move()
    local c = Camera.new()
    camera.set(c)
    local a = 10 * camera.MAX_VEL
    local cmp = function(t) common.assert_eq({c:acc()}, t, common.deep_cmp) end
    camera.move(0, true, 0, {1, 0, 0, 0, 0, 0})
    cmp{-a, 0, 0}
    camera.move(0, false, 0, {0, 0, 0, 0, 0, 0})
    cmp{0, 0, 0}
    camera.move(0, true, 0, {0, 1, 0, 0, 0, 0})
    cmp{a, 0, 0}
    camera.move(0, false, 0, {0, 0, 0, 0, 0, 0})
    cmp{0, 0, 0}
    camera.move(0, true, 0, {0, 0, 1, 0, 0, 0})
    cmp{0, -a, 0}
    camera.move(0, false, 0, {0, 0, 0, 0, 0, 0})
    cmp{0, 0, 0}
    camera.move(0, true, 0, {0, 0, 0, 1, 0, 0})
    cmp{0, a, 0}
    camera.move(0, false, 0, {0, 0, 0, 0, 0, 0})
    cmp{0, 0, 0}
    camera.move(0, true, 0, {0, 0, 0, 0, 1, 0})
    cmp{0, 0, -a}
    camera.move(0, false, 0, {0, 0, 0, 0, 0, 0})
    cmp{0, 0, 0}
    camera.move(0, true, 0, {0, 0, 0, 0, 0, 1})
    cmp{0, 0, a}
    camera.move(0, false, 0, {0, 0, 0, 0, 0, 0})
    cmp{0, 0, 0}
end

local function test_move_zoom()
    local c = Camera.new()
    camera.set(c)
    camera.move(
        Input.KEY_PAGE_DOWN, true, Input.MOD_ALT, {0, 0, 0, 0, 0, 1})
    common.assert_eq(c:zoom_acc(), camera.ZOOM_VEL)
    camera.move(
        Input.KEY_PAGE_DOWN, false, Input.MOD_ALT, {0, 0, 0, 0, 0, 0})
    common.assert_eq(c:zoom_acc(), 0)
end

local function test_move_rot()
    local c = Camera.new()
    camera.set(c)
    local a = camera.ROT_VEL
    local cmp = function(t)
        common.assert_eq({c:rot_acc()}, t, common.deep_cmp)
    end
    camera.move(0, true, Input.MOD_SHIFT, {1, 0, 0, 0, 0, 0})
    cmp{0, a, 0}
    camera.move(0, false, Input.MOD_SHIFT, {0, 0, 0, 0, 0, 0})
    cmp{0, 0, 0}
    camera.move(0, true, Input.MOD_SHIFT, {0, 1, 0, 0, 0, 0})
    cmp{0, -a, 0}
    camera.move(0, false, Input.MOD_SHIFT, {0, 0, 0, 0, 0, 0})
    cmp{0, 0, 0}
    camera.move(0, true, Input.MOD_SHIFT, {0, 0, 1, 0, 0, 0})
    cmp{a, 0, 0}
    camera.move(0, false, Input.MOD_SHIFT, {0, 0, 0, 0, 0, 0})
    cmp{0, 0, 0}
    camera.move(0, true, Input.MOD_SHIFT, {0, 0, 0, 1, 0, 0})
    cmp{-a, 0, 0}
    camera.move(0, false, Input.MOD_SHIFT, {0, 0, 0, 0, 0, 0})
    cmp{0, 0, 0}
    camera.move(0, true, Input.MOD_SHIFT, {0, 0, 0, 0, 1, 0})
    cmp{0, 0, a}
    camera.move(0, false, Input.MOD_SHIFT, {0, 0, 0, 0, 0, 0})
    cmp{0, 0, 0}
    camera.move(0, true, Input.MOD_SHIFT, {0, 0, 0, 0, 0, 1})
    cmp{0, 0, -a}
    camera.move(0, false, Input.MOD_SHIFT, {0, 0, 0, 0, 0, 0})
    cmp{0, 0, 0}
end

local function test_move_dash()
    local c = Camera.new()
    camera.set(c)
    local v = camera.MAX_VEL
    local a, r = 10 * v, v / 50
    local key_a, key_pgdown = string.byte("A"), Input.KEY_PAGE_DOWN
    utils.reset_double_tap()
    camera.move(key_a, true, 0, {1, 0, 0, 0, 0, 0})
    assert(not c:dash())
    camera.move(key_a, true, 0, {1, 0, 0, 0, 0, 0})
    assert(c:dash())
    c:set_acc(0, 0, 0)
    c:set_dash(false)
    camera.move(key_a, true, Input.MOD_SHIFT, {1, 0, 0, 0, 0, 0})
    assert(not c:dash())
    camera.move(key_a, true, Input.MOD_SHIFT, {1, 0, 0, 0, 0, 0})
    assert(c:dash())
    c:set_rot_acc(0, 0, 0)
    c:set_dash(false)
    camera.move(key_pgdown, true, Input.MOD_ALT, {0, 0, 0, 0, 1, 0})
    assert(not c:dash())
    camera.move(key_pgdown, true, Input.MOD_ALT, {0, 0, 0, 0, 1, 0})
    assert(c:dash())
    utils.reset_double_tap()
end

common.setup_hook(1)
test_reset_ortho()
test_reset_persp()
test_move()
test_move_zoom()
test_move_rot()
test_move_dash()
nngn:exit()
