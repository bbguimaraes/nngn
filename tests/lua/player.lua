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
    player.remove(nngn.players:add(e))
    common.assert_eq(nngn.entities:n(), 0)
    common.assert_eq(nngn.players:n(), 0)
    player.set(nil)
end

local function test_remove()
    local e = nngn.entities:add()
    local p = nngn.players:add(e)
    player.remove(p)
    common.assert_eq(nngn.entities:n(), 0)
    common.assert_eq(nngn.players:n(), 0)
end

local function test_next()
    local e0 = nngn.entities:add()
    local p0, p1, p2 =
        nngn.players:add(e0), nngn.players:add(), nngn.players:add()
    common.assert_eq(nngn.players:cur(), p0)
    e0:set_vel(1, 2, 3)
    player.next()
    common.assert_eq({e0:vel()}, {0, 0, 0}, common.deep_cmp)
    common.assert_eq(nngn.players:cur(), p1)
    player.next()
    common.assert_eq(nngn.players:cur(), p2)
    player.next()
    common.assert_eq(nngn.players:cur(), p0)
    player.next()
    common.assert_eq(nngn.players:cur(), p1)
    player.next(-1)
    common.assert_eq(nngn.players:cur(), p0)
    nngn.players:remove(p2)
    nngn.players:remove(p1)
    nngn.players:remove(p0)
end

local function test_move()
    local e = nngn.entities:add()
    local p = nngn.players:add(e)
    local a = player.MAX_VEL
    local cmp = function(t) common.assert_eq({e:vel()}, t, common.deep_cmp) end
    player.move(nil, nil, nil, {1, 0, 0, 0})
    cmp{-a, 0, 0}
    common.assert_eq(p:face(), Player.FLEFT)
    player.move(nil, nil, nil, {0, 0, 0, 0})
    cmp{0, 0, 0}
    common.assert_eq(p:face(), Player.FLEFT)
    player.move(nil, nil, nil, {0, 1, 0, 0})
    cmp{a, 0, 0}
    common.assert_eq(p:face(), Player.FRIGHT)
    player.move(nil, nil, nil, {0, 0, 0, 0})
    cmp{0, 0, 0}
    common.assert_eq(p:face(), Player.FRIGHT)
    player.move(nil, nil, nil, {0, 0, 1, 0})
    cmp{0, -a, 0}
    common.assert_eq(p:face(), Player.FDOWN)
    player.move(nil, nil, nil, {0, 0, 0, 0})
    cmp{0, 0, 0}
    common.assert_eq(p:face(), Player.FDOWN)
    player.move(nil, nil, nil, {0, 0, 0, 1})
    cmp{0, a, 0}
    common.assert_eq(p:face(), Player.FUP)
    player.move(nil, nil, nil, {0, 0, 0, 0})
    cmp{0, 0, 0}
    common.assert_eq(p:face(), Player.FUP)
    player.remove(p)
end

local function test_move_diag()
    local e = nngn.entities:add()
    local p = nngn.players:add(e)
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
    common.assert_eq(p:face(), Player.FLEFT)
    player.move(nil, nil, nil, {1, 0, 1, 0})
    cmp(-a2, -a2)
    common.assert_eq(p:face(), Player.FLEFT)
    player.move(nil, nil, nil, {0, 0, 1, 0})
    cmp(0, -a)
    common.assert_eq(p:face(), Player.FDOWN)
    player.move(nil, nil, nil, {0, 1, 1, 0})
    cmp(a2, -a2)
    common.assert_eq(p:face(), Player.FDOWN)
    player.move(nil, nil, nil, {0, 1, 0, 0})
    cmp(a, 0)
    common.assert_eq(p:face(), Player.FRIGHT)
    player.move(nil, nil, nil, {0, 1, 0, 1})
    cmp(a2, a2)
    common.assert_eq(p:face(), Player.FRIGHT)
    player.move(nil, nil, nil, {0, 0, 0, 1})
    cmp(0, a)
    common.assert_eq(p:face(), Player.FUP)
    player.move(nil, nil, nil, {1, 0, 0, 1})
    cmp(-a2, a2)
    common.assert_eq(p:face(), Player.FUP)
    player.move(nil, nil, nil, {1, 0, 0, 0})
    cmp(-a, 0)
    common.assert_eq(p:face(), Player.FLEFT)
    player.move(nil, nil, nil, {0, 0, 0, 0})
    cmp(0, 0)
    common.assert_eq(p:face(), Player.FLEFT)
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
