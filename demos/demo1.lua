dofile "src/lua/init.lua"
dofile "src/lua/limits.lua"
dofile "src/lua/main.lua"
local entity = require "nngn.lib.entity"
local input = require "nngn.lib.input"
local light = require "nngn.lib.light"
local map = require "nngn.lib.map"
local player = require "nngn.lib.player"
local textbox = require "nngn.lib.textbox"

DEMO = {i = 1, stages = {}, data = {}}

function DEMO:add_stage(text, f)
    text = string.gsub(
        text, "<([^>]+)>",
        Textbox.TEXT_BLUE_STR .. "%1" .. Textbox.TEXT_WHITE_STR)
    table.insert(self.stages, {text, f})
end

function DEMO:next()
    local i, stages = self.i, self.stages
    if i > #stages then return end
    local text, f = table.unpack(stages[i])
    if f then f() end
    textbox.update("nngn", text)
    self.i = i + 1
end

DEMO:add_stage [[
This is nngn, a 2D/3D graphics/physics/game
engine (press <Enter> to advance the demo).]]

DEMO:add_stage [[
More information, including links to the
source, can be found at:
https://bbguimaraes.com/nngn.]]

DEMO:add_stage [[
It is free and open source software, so it
can be used by anyone for anything.]]

DEMO:add_stage [[
It was written from scratch and features:
- Lua scripting
- OpenGL/Vulkan rendering
- OpenCL hardware acceleration
- native and WebAssembly versions]]

DEMO:add_stage "This demo will show some of the controls."

DEMO:add_stage [[
The window title shows:
- current/average FPS
- minimum/maximum frame time (ms)]]

DEMO:add_stage [[
The default swap interval is 1 (vsync).
<Ctrl-Shift-i> will decrease it to 0 (unbounded),
<Ctrl-i> increases it.]]

DEMO:add_stage [[
Fast-forward/dismiss text boxes using <Space>.]]

DEMO:add_stage "Add players using <Ctrl-p>."

DEMO:add_stage "Move between existing players with <Tab>."

DEMO:add_stage [[
Cycle through the available players with
<Ctrl-Alt-p>.]]

DEMO:add_stage [[
Remove players with <Ctrl-Shift-p> (adding <Shift>
to a key combination usually reverses it).]]

DEMO:add_stage [[
Players can walk (<AWSD>) and run (pressing
the key twice quickly).]]

DEMO:add_stage [[
Time can be scaled using <Ctrl-v>. Pressing it
slows time down. <Ctrl-Shift-v> moves in the
other direction. There are a few predefined
multipliers, which eventually wrap around.]]

DEMO:add_stage [[
The camera follows the current player, but
can be controlled separately (using the <arrow
keys>).]]

DEMO:add_stage [[
Press <C> to make the camera follow the player
again or to toggle.]]

DEMO:add_stage [[
The camera can also be zoomed in and out
using <Alt-PageUp>/<PageDown>.]]

DEMO:add_stage "Reset the camera using <Ctrl-c>."

DEMO:add_stage [[
The default camera uses an ortographic
projection. Z values are respected, however,
for a sense of depth (add another player and
walk behind it).]]

DEMO:add_stage [[
Press <Ctrl-Alt-c> to change to a perspective
camera. The position is maintained, so press
<Ctrl-c> to set it to the default position for
a better view.]]

DEMO:add_stage [[
In perspective mode, <PageUp>/<PageDown>
move the camera along the third axis.]]

DEMO:add_stage [[
Press <Ctrl-Alt-c> and <Ctrl-c> to go back to the
ortographic view.]]

DEMO:add_stage([[
All static and animated objects are rendered
using tile sheets.]],
    function()
        DEMO.data.tilesheet0 = entity.load(nil, nil, {
            pos = {-128, 128},
            renderer = {
                type = Renderer.SPRITE,
                size = {512, 512},
                tex = "img/chrono_trigger/crono.png"}})
        DEMO.data.tilesheet1 = entity.load(nil, nil, {
            pos = {352, 128},
            renderer = {
                type = Renderer.SPRITE,
                size = {512, 512},
                tex = "img/zelda.png"}})
    end)

DEMO:add_stage(
    "Let us move to a busier place...",
    function()
        nngn:remove_entity(DEMO.data.tilesheet0)
        nngn:remove_entity(DEMO.data.tilesheet1)
        DEMO.data.tilesheet0 = nil
        DEMO.data.tilesheet1 = nil
    end)

DEMO:add_stage(
    "A map is now rendered (toggle with <Ctrl-m>).",
    function() dofile("maps/main.lua") end)

DEMO:add_stage [[
The engine has collision detection and
resolution, either via native code or
OpenCL.]]

DEMO:add_stage [[
Several collider types are supported and
most are present here.]]

DEMO:add_stage [[
<Ctrl-b> cycles through some debug
visualizations. Press it six times to view all
the ones related to collisions.]]

DEMO:add_stage [[
Red/green boxes are axis-aligned and rotated
bounding boxes (with their respective
bounding circles.]]

DEMO:add_stage "Circles by themselves are sphere colliders."

DEMO:add_stage [[
Press <Ctrl-Shift-b> also six times to cycle
back to no debug visualizations.]]

DEMO:add_stage [[
Collision can be disabled for debug by
pressing <Ctrl-o>.]]

DEMO:add_stage [[
A grid, which is very useful for debugging
and placing entities on the map, can be
shown by pressing <Ctrl-g>.]]

DEMO:add_stage "Let us move to a darker place..."

DEMO:add_stage([[
The engine has dynamic lighting. At the
moment, only a dim ambient light exists.]],
    function()
        dofile("maps/wolfenstein.lua")
        nngn.lighting:set_shadows_enabled(false)
    end)

DEMO:add_stage [[
Increase and decrease the ambient light
with <Ctrl-n>/<Ctrl-Shift-n>.]]

DEMO:add_stage "Toggle the player's light with <L>."

DEMO:add_stage [[
This is a point light. Toggle a flashlight-like
spotlight with <F>.]]

DEMO:add_stage [[
Shadows are currently disable. Toggle them
with <Ctrl-s>.]]

DEMO:add_stage [[
Shadows are completely dynamic and
pixel-perfect (see the sprite shadows).]]

DEMO:add_stage [[
Lights are not just static, they can also be
animated. Explore effects that use this
using <H> and <E>.]]

DEMO:add_stage "Let us move to an open space..."

DEMO:add_stage([[
The final type of light supported by the
engine is directional lights, most commonly
used to simulate the light of the sun.]],
    function()
        dofile("maps/hm.lua")
        player.move_all(0, 0, true)
        DEMO.data.hm_wall = entity.load(nil, nil, {
            pos = {-112, 16},
            collider = {
                type = Collider.AABB,
                bb = {32, 64}, m = Math.INFINITY, flags = Collider.SOLID}})
    end)

DEMO:add_stage [[
By itself, it is not very spectacular. Another
lighting effect the engine supports
is Z sprites, combining the ortographic
rendering of sprites with the perspective
lighting effects.]]

DEMO:add_stage [[
The complete setup:
- enable Z sprites with <Ctrl-z>
- decrease ambient light with <Ctrl-Shift-n>
- enable the sun with <Ctrl-Alt-n>
- enable shadows with <Ctrl-s>]]

DEMO:add_stage [[
We'll now move back to the main map and
re-enable warps so you can explore.]]

DEMO:add_stage(
    "Thanks for playing with this demo.",
    function()
        nngn:remove_entity(DEMO.data.hm_wall)
        DEMO.data.hm_wall = nil
        map.next = DEMO.data.map_next
        DEMO.data.map_next = nil
        light.sun(false)
        nngn.lighting:set_ambient_light(1, 1, 1, 1)
        nngn.lighting:set_shadows_enabled(false)
        nngn.lighting:set_zsprites(false)
        nngn.renderers:set_zsprites(false)
        dofile("maps/main.lua")
    end)

input.input:add(
    Input.KEY_ENTER, Input.SEL_PRESS,
    function() DEMO:next() end)
DEMO.data.map_next = map.next
map.next = function() end
DEMO:next()
