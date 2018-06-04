dofile "src/lua/path.lua"
local common = require "tests/lua/common"
local entity = require "nngn.lib.entity"
local nngn_math = require "nngn.lib.math"
local player = require "nngn.lib.player"

local function test_load()
    player.set("src/lson/crono.lua")
    local e = nngn.entities:add()
    player.load(e)
    common.assert_eq(nngn.entities:name(e), "crono")
    player.remove(player.add(e))
    common.assert_eq(nngn.entities:n(), 0)
    common.assert_eq(player.n(), 0)
    player.set(nil)
end

local function test_remove()
    local e = nngn.entities:add()
    local p = player.add(e)
    player.remove(p)
    common.assert_eq(nngn.entities:n(), 0)
    common.assert_eq(player.n(), 0)
end

local function test_next()
    local e0 = nngn.entities:add()
    local p0, p1, p2 = player.add(e0), player.add(), player.add()
    common.assert_eq(player.cur(), p0)
    e0:set_vel(1, 2, 3)
    player.next()
    common.assert_eq({e0:vel()}, {0, 0, 0}, common.deep_cmp)
    common.assert_eq(player.cur(), p1)
    player.next()
    common.assert_eq(player.cur(), p2)
    player.next()
    common.assert_eq(player.cur(), p0)
    player.next()
    common.assert_eq(player.cur(), p1)
    player.next(-1)
    common.assert_eq(player.cur(), p0)
    player.remove(p2)
    player.remove(p1)
    player.remove(p0)
end

local function test_move()
    local e = nngn.entities:add()
    local p = player.add(e)
    local v = player.MAX_VEL
    local cmp = function(t) common.assert_eq({e:vel()}, t, common.deep_cmp) end
    player.move(nil, nil, nil, {1, 0, 0, 0})
    cmp{-v, 0, 0}
    common.assert_eq(p.face, player.FACE.LEFT)
    player.move(nil, nil, nil, {0, 0, 0, 0})
    cmp{0, 0, 0}
    common.assert_eq(p.face, player.FACE.LEFT)
    player.move(nil, nil, nil, {0, 1, 0, 0})
    cmp{v, 0, 0}
    common.assert_eq(p.face, player.FACE.RIGHT)
    player.move(nil, nil, nil, {0, 0, 0, 0})
    cmp{0, 0, 0}
    common.assert_eq(p.face, player.FACE.RIGHT)
    player.move(nil, nil, nil, {0, 0, 1, 0})
    cmp{0, -v, 0}
    common.assert_eq(p.face, player.FACE.DOWN)
    player.move(nil, nil, nil, {0, 0, 0, 0})
    cmp{0, 0, 0}
    common.assert_eq(p.face, player.FACE.DOWN)
    player.move(nil, nil, nil, {0, 0, 0, 1})
    cmp{0, v, 0}
    common.assert_eq(p.face, player.FACE.UP)
    player.move(nil, nil, nil, {0, 0, 0, 0})
    cmp{0, 0, 0}
    common.assert_eq(p.face, player.FACE.UP)
    player.remove(p)
end

local function test_move_diag()
    local e = nngn.entities:add()
    local p = player.add(e)
    local a = player.MAX_VEL
    local a2 = a / math.sqrt(2)
    local float_eq = function(x, y)
        return nngn_math.float_eq(x, y, 8 * Math.FLOAT_EPSILON)
    end
    local cmp = function(ex, ey)
        local x, y = e:vel()
        common.assert_eq(x, ex, float_eq)
        common.assert_eq(y, ey, float_eq)
    end
    player.move(nil, nil, nil, {1, 0, 0, 0})
    cmp(-a, 0)
    common.assert_eq(p.face, player.FACE.LEFT)
    player.move(nil, nil, nil, {1, 0, 1, 0})
    cmp(-a2, -a2)
    common.assert_eq(p.face, player.FACE.LEFT)
    player.move(nil, nil, nil, {0, 0, 1, 0})
    cmp(0, -a)
    common.assert_eq(p.face, player.FACE.DOWN)
    player.move(nil, nil, nil, {0, 1, 1, 0})
    cmp(a2, -a2)
    common.assert_eq(p.face, player.FACE.DOWN)
    player.move(nil, nil, nil, {0, 1, 0, 0})
    cmp(a, 0)
    common.assert_eq(p.face, player.FACE.RIGHT)
    player.move(nil, nil, nil, {0, 1, 0, 1})
    cmp(a2, a2)
    common.assert_eq(p.face, player.FACE.RIGHT)
    player.move(nil, nil, nil, {0, 0, 0, 1})
    cmp(0, a)
    common.assert_eq(p.face, player.FACE.UP)
    player.move(nil, nil, nil, {1, 0, 0, 1})
    cmp(-a2, a2)
    common.assert_eq(p.face, player.FACE.UP)
    player.move(nil, nil, nil, {1, 0, 0, 0})
    cmp(-a, 0)
    common.assert_eq(p.face, player.FACE.LEFT)
    player.move(nil, nil, nil, {0, 0, 0, 0})
    cmp(0, 0)
    common.assert_eq(p.face, player.FACE.LEFT)
    player.remove(p)
end

nngn:set_graphics(Graphics.PSEUDOGRAPH)
nngn.entities:set_max(2)
nngn.graphics:resize_textures(2)
nngn.textures:set_max(2)
nngn.renderers:set_max_sprites(1)
common.setup_hook(1)
test_load()
test_remove()
test_move()
test_move_diag()
nngn:exit()
