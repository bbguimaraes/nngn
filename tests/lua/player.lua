dofile "src/lua/path.lua"
local common = require "tests/lua/common"
local entity = require "nngn.lib.entity"
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
    local p0, p1, p2 =
        nngn.players:add(), nngn.players:add(), nngn.players:add()
    common.assert_eq(nngn.players:cur(), p0)
    player.next()
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

nngn:set_graphics(Graphics.PSEUDOGRAPH)
nngn.entities:set_max(2)
nngn.graphics:resize_textures(2)
nngn.textures:set_max(2)
nngn.renderers:set_max_sprites(1)
common.setup_hook(1)
test_load()
test_remove()
nngn:exit()
