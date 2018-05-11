dofile "src/lua/path.lua"
local common = require "tests/lua/common"
local entity = require "nngn.lib.entity"
local player = require "nngn.lib.player"

local function test_load()
    player.set("src/lson/crono.lua")
    local e = nngn:entities():add()
    player.load(e)
    common.assert_eq(nngn:entities():name(e), "crono")
    player.remove(player.add(e))
    common.assert_eq(nngn:entities():n(), 0)
    common.assert_eq(player.n(), 0)
    player.set(nil)
end

local function test_remove()
    local e = nngn:entities():add()
    local p = player.add(e)
    player.remove(p)
    common.assert_eq(nngn:entities():n(), 0)
    common.assert_eq(player.n(), 0)
end

local function test_next()
    local p0, p1, p2 = player.add(), player.add(), player.add()
    common.assert_eq(player.cur(), p0)
    player.next()
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

nngn:set_graphics(Graphics.PSEUDOGRAPH)
nngn:entities():set_max(2)
nngn:graphics():resize_textures(2)
nngn:textures():set_max(2)
nngn:renderers():set_max_sprites(1)
common.setup_hook(1)
test_load()
test_remove()
nngn:exit()
