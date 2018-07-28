dofile "src/lua/path.lua"
local camera = require "nngn.lib.camera"
local common = require "tests/lua/common"
local entity = require "nngn.lib.entity"
local nngn_math = require "nngn.lib.math"
local player = require "nngn.lib.player"
local utils = require "nngn.lib.utils"

local function test_load()
    player.set{
        "src/lson/crono.lua",
        "src/lson/zelda/link.lua",
        "src/lson/zelda/link_sh.lua",
    }
    local e = nngn:entities():add()
    player.load(e, true)
    common.assert_eq(nngn:entities():name(e), "crono")
    common.assert_eq(e:animation():sprite():cur_track(), player.FACE.DOWN)
    player.load(e, true)
    common.assert_eq(nngn:entities():name(e), "link")
    player.load(e, true)
    common.assert_eq(nngn:entities():name(e), "link_sh")
    player.remove(player.add(e))
    common.assert_eq(nngn:entities():n(), 0)
    common.assert_eq(player.n(), 0)
    player.set(nil)
end

local function test_remove()
    local e = nngn:entities():add()
    local p = player.add(e)
    camera.set_follow(e)
    player.remove(p)
    common.assert_eq(nngn:entities():n(), 0)
    common.assert_eq(player.n(), 0)
    common.assert_eq(camera.following(), nil)
end

local function test_stop()
    local e = entity.load(nil, "src/lson/crono.lua")
    local a = e:animation():sprite()
    local p = player.add(e)
    e:set_vel(1, 2, 3)
    player.stop(p)
    common.assert_eq({e:vel()}, {0, 0, 0}, common.deep_cmp)
    a:set_track(player.ANIMATION.WLEFT)
    player.stop(p)
    common.assert_eq(a:cur_track(), player.ANIMATION.FLEFT)
    a:set_track(player.ANIMATION.WRIGHT)
    player.stop(p)
    common.assert_eq(a:cur_track(), player.ANIMATION.FRIGHT)
    a:set_track(player.ANIMATION.WDOWN)
    player.stop(p)
    common.assert_eq(a:cur_track(), player.ANIMATION.FDOWN)
    a:set_track(player.ANIMATION.WUP)
    player.stop(p)
    common.assert_eq(a:cur_track(), player.ANIMATION.FUP)
    p.running = true
    player.stop(p)
    assert(not p.running)
    player.remove(p)
end

local function test_next()
    local t = dofile("src/lua/crono.lua")
    local e0 = entity.load(nil, nil, t)
    local e1 = entity.load(nil, nil, t)
    local e2 = entity.load(nil, nil, t)
    local p0, p1, p2 = player.add(e0), player.add(e1), player.add(e2)
    common.assert_eq(player.cur(), p0)
    assert(nngn:renderers():selected(), e0:renderer())
    e0:set_vel(1, 2, 3)
    player.next()
    common.assert_eq({e0:vel()}, {0, 0, 0}, common.deep_cmp)
    common.assert_eq(player.cur(), p1)
    assert(nngn:renderers():selected(), e1:renderer())
    player.next()
    common.assert_eq(player.cur(), p2)
    assert(nngn:renderers():selected(), e2:renderer())
    player.next()
    common.assert_eq(player.cur(), p0)
    assert(nngn:renderers():selected(), e0:renderer())
    player.next()
    common.assert_eq(player.cur(), p1)
    assert(nngn:renderers():selected(), e1:renderer())
    player.next(-1)
    common.assert_eq(player.cur(), p0)
    assert(nngn:renderers():selected(), e0:renderer())
    player.remove(p2)
    player.remove(p1)
    player.remove(p0)
end

local function test_move()
    local e = entity.load(nil, "src/lson/crono.lua")
    local a = e:animation():sprite()
    local p = player.add(e)
    local v = player.MAX_VEL
    local cmp = function(t) common.assert_eq({e:vel()}, t, common.deep_cmp) end
    player.move(nil, nil, nil, {1, 0, 0, 0})
    cmp{-v, 0, 0}
    common.assert_eq(p.face, player.FACE.LEFT)
    common.assert_eq(a:cur_track(), player.ANIMATION.WLEFT)
    player.move(nil, nil, nil, {0, 0, 0, 0})
    cmp{0, 0, 0}
    common.assert_eq(p.face, player.FACE.LEFT)
    common.assert_eq(a:cur_track(), player.ANIMATION.FLEFT)
    player.move(nil, nil, nil, {0, 1, 0, 0})
    cmp{v, 0, 0}
    common.assert_eq(p.face, player.FACE.RIGHT)
    common.assert_eq(a:cur_track(), player.ANIMATION.WRIGHT)
    player.move(nil, nil, nil, {0, 0, 0, 0})
    cmp{0, 0, 0}
    common.assert_eq(p.face, player.FACE.RIGHT)
    common.assert_eq(a:cur_track(), player.ANIMATION.FRIGHT)
    player.move(nil, nil, nil, {0, 0, 1, 0})
    cmp{0, -v, 0}
    common.assert_eq(p.face, player.FACE.DOWN)
    common.assert_eq(a:cur_track(), player.ANIMATION.WDOWN)
    player.move(nil, nil, nil, {0, 0, 0, 0})
    cmp{0, 0, 0}
    common.assert_eq(p.face, player.FACE.DOWN)
    common.assert_eq(a:cur_track(), player.ANIMATION.FDOWN)
    player.move(nil, nil, nil, {0, 0, 0, 1})
    cmp{0, v, 0}
    common.assert_eq(p.face, player.FACE.UP)
    common.assert_eq(a:cur_track(), player.ANIMATION.WUP)
    player.move(nil, nil, nil, {0, 0, 0, 0})
    cmp{0, 0, 0}
    common.assert_eq(p.face, player.FACE.UP)
    common.assert_eq(a:cur_track(), player.ANIMATION.FUP)
    player.remove(p)
end

local function test_move_diag()
    local e = nngn:entities():add()
    local p = player.add(e)
    local a = player.MAX_VEL
    local a2 = a / math.sqrt(2)
    local float_eq = function(x, y)
        return nngn_math.float_eq(x, y, 32 * Math.FLOAT_EPSILON)
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

local function test_move_running()
    utils.reset_double_tap()
    local key = string.byte("A")
    local e = entity.load(nil, "src/lson/crono.lua")
    local a = e:animation():sprite()
    local p = player.add(e)
    p.running = true
    player.move(key, true, nil, {1, 0, 0, 0})
    player.move(key, true, nil, {1, 0, 0, 0})
    common.assert_eq(a:cur_track(), player.ANIMATION.RLEFT)
    player.move(key, false, nil, {0, 0, 0, 0})
    assert(not p.running)
    player.remove(p)
end

nngn:set_graphics(Graphics.PSEUDOGRAPH)
nngn:entities():set_max(2)
nngn:graphics():resize_textures(3)
nngn:textures():set_max(3)
nngn:renderers():set_max_sprites(1)
nngn:animations():set_max(1)
common.setup_hook(1)
test_load()
test_remove()
test_stop()
test_move()
test_move_diag()
test_move_running()
nngn:exit()
