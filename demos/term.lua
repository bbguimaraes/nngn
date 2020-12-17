nngn:set_graphics(Graphics.TERMINAL_BACKEND)
dofile "src/lua/all.lua"

local animation = require "nngn.lib.animation"
local camera = require "nngn.lib.camera"
local player = require "nngn.lib.player"

camera.reset(4)

nngn.input:add_source(Input.terminal_source(0))
nngn.colliders:set_resolve(false)
local p = player.add()
local e = p:entity()
local anims = {
    animation.cycle{
        animation.sprite(e, Player.WLEFT),
        animation.velocity(e, {-128, 0, 0}, {-64, 0, 0}),
        animation.sprite(e, Player.WRIGHT),
        animation.velocity(e, {   0, 0, 0}, { 64, 0, 0}),
    },
    animation.cycle{
        animation.camera({-128, 0, 0}, {-64, 0, 0}),
        animation.camera({   0, 0, 0}, { 64, 0, 0}),
    },
}
nngn.schedule:next(Schedule.HEARTBEAT, function()
    for i, a in ipairs(anims) do
        anims[i] = a:update(nngn.timing:dt_ms())
    end
end)
