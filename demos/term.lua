dofile "src/lua/path.lua"
require("nngn.lib.graphics").init{"terminal_colored"}
dofile "src/lua/all.lua"
local animation <const> = require "nngn.lib.animation"
local camera <const> = require "nngn.lib.camera"
local player <const> = require "nngn.lib.player"
local textbox <const> = require "nngn.lib.textbox"

local p <const> = player.add()
local e <const> = p.entity
local anims <const> = {
    animation.cycle{
        animation.sprite(e, player.ANIMATION.WLEFT),
        animation.velocity(e, {-128, 0, 0}, {-64, 0, 0}),
        animation.sprite(e, player.ANIMATION.WRIGHT),
        animation.velocity(e, {   0, 0, 0}, { 64, 0, 0}),
    },
    animation.cycle{
        animation.camera({-128, 0, 0}, {-64, 0, 0}),
        animation.camera({   0, 0, 0}, { 64, 0, 0}),
    },
}

nngn:graphics():set_swap_interval(1)
camera.reset(8)
camera.set_follow(false)
require("nngn.lib.font").load(64)
nngn:input():add_source(Input.terminal_source(0))
nngn:colliders():set_resolve(false)
local f = io.open("test.txt", "a")
f:setvbuf("line")
nngn:schedule():next(Schedule.HEARTBEAT, function()
    f:write(tostring(nngn:fps():dump().avg), "\n")
    textbox.update(
        "resolution",
        string.format("%d x %d", nngn:graphics():window_size()))
    for i, a in ipairs(anims) do
        anims[i] = a:update(nngn:timing():dt_ms())
    end
end)
function demo_start() end
