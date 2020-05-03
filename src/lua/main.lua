local camera = require "nngn.lib.camera"
local entity = require "nngn.lib.entity"
local font = require "nngn.lib.font"
local map = require "nngn.lib.map"
local player = require "nngn.lib.player"
local texture = require "nngn.lib.texture"
dofile "src/lua/input.lua"

nngn.input:register_callback(map.key_callback)

function on_collision(e0, e1, ...)
    map.on_collision(e0, e1, ...)
    local fire = player.data(nil, "fire")
    if fire then
        local e = fire.entity
        if e and (e0 == e or e1 == e) then fire(FIRE_CMD_HIT) end
    end
end

camera.reset()
player.set{
    "src/lson/crono.lua", "src/lson/link.lua", "src/lson/link_sh.lua",
    "src/lson/fairy0.lua", "src/lson/chocobo.lua", "src/lson/null.lua"}
nngn.textures:load(texture.NNGN)
font.load(32)
nngn.grid:set_dimensions(32.0, 64)

dofile("src/lua/fire.lua")
