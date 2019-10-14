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
        if e and (e0 == e or e1 == e) then fire.remove() end
    end
end

camera.reset()
player.set{
    "src/lson/crono.lua", "src/lson/link.lua", "src/lson/link_sh.lua",
    "src/lson/fairy0.lua", "src/lson/chocobo.lua", "src/lson/null.lua"}
nngn.textures:load(texture.NNGN)
font.load(32)
nngn.grid:set_dimensions(32.0, 64)
nngn.colliders:set_resolve(false)

nngn.schedule:next(Schedule.HEARTBEAT, function()
    local p = nngn.players:cur()
    if not p or camera.following() then return end
    local m = {nngn.graphics:mouse_pos()}
    local s = {nngn.graphics:window_size()}
    print("m", table.unpack(m))
    print("w", nngn.camera:to_world(m[1], m[2], 0))
    m = {nngn.camera:to_world(m[1], m[2], 0)}
--    local z = nngn.camera:zoom()
--    m[1] = (m[1] - s[1] / 2) / z
--    m[2] = (m[2] - s[2] / 2) / z
    p:entity():set_pos(table.unpack(m))
end)
