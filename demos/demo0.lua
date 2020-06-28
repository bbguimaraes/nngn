dofile "src/lua/init.lua"
dofile "src/lua/limits.lua"
dofile "src/lua/main.lua"
dofile "maps/main.lua"
local input = require "nngn.lib.input"
local textbox = require "nngn.lib.textbox"

DEMO = {i = 1, stages = {}}

function DEMO:add_stage(text) table.insert(self.stages, text) end

function DEMO:next()
    local i, stages = self.i, self.stages
    if i > #stages then return end
    textbox.set("nngn", self.stages[i])
    self.i = i + 1
end

DEMO:add_stage "This is nngn, a 2d simulation/game engine."

DEMO:add_stage [[
The source can be found at:
https://github.com/bbguimaraes/nngn.git]]

DEMO:add_stage [[
It is free and open source software, so it
can be used by anyone for anything.]]

DEMO:add_stage [[
It was written from scratch and uses opengl's
programmable pipeline for rendering and lua
for scripting (more on that later).]]

DEMO:add_stage [[
And because 1) I am a masochist,
2) emscripten is awesome, and 3) javascript,
it can actually be tried out without installing
or compiling anything just by going to:
https://bbguimaraes.com/nngn]]

DEMO:add_stage "As you can see, it renders at constant 60 fps."

DEMO:add_stage [[
And if it's not obvious, all the graphics are
ripped from other games, used here for no
commercial purpose and this video claims no
copyright over them.]]

DEMO:add_stage [[
This is a player, added by pressing the 'p'
key.]]

DEMO:add_stage [[
Players can walk (AWSD) and run (pressing
the key twice quickly).]]

DEMO:add_stage [[
Time can be scaled (which originally was
meant to be used to debug animations, but
is actually pretty cool).]]

DEMO:add_stage [[
The camera follows the current player, but
can be controlled separatly (using the arrow
keys).]]

DEMO:add_stage [[
It can also zoom in and out (using the page
up/down keys).]]

DEMO:add_stage [[
To look at this beautiful lamp post animation,
for example.]]

DEMO:add_stage [[
This map is rendered as a single image (as
most Chrono Trigger maps, it's quite
complex).]]

DEMO:add_stage [[
There are some entities added on top of it
to give a sense of depth.]]

DEMO:add_stage [[
As can be seen when the player walks behind
that fence or this lamp post.]]

DEMO:add_stage [[
The other way of rendering a map is using a
tilesheet.]]

DEMO:add_stage "This is a test map for a tilesheet."

DEMO:add_stage [[
This is another test map for a tilesheet (the
same tilesheet, actually).]]

DEMO:add_stage [[
All static and animated objects are also
rendered using tilesheets.]]

DEMO:add_stage [[
Any resemblance to a house in another
game is pure coincidence.]]

DEMO:add_stage [[
There is basic collision detection that
can be used to create solids.]]

DEMO:add_stage [[
It is based on bounding boxes and supports
both axis-aligned and rotated boxes.]]

DEMO:add_stage [[
Bounding boxes can be shown by pressing
the 'b' key.]]

DEMO:add_stage [[
Collision can be disabled for debug by
pressing the 'o' key.]]

DEMO:add_stage [[
There's also a grid, which is very useful for
debugging and placing entities on the map.
It can be shown by pressing the 'g' key.]]

DEMO:add_stage [[
There's support for multiple players, which
can be added by pressing the 'p' key again.]]

DEMO:add_stage [[
Players can be switched to by pressing the
'tab' key.]]

DEMO:add_stage [[
The player can be changed by pressing the
'l' key.]]

DEMO:add_stage [[
The engine is responsible for the lower level
tasks of rendering, physics, etc., but most
of the perceived behavior is actually
programmed in lua.]]

DEMO:add_stage [[
That means it can be easily modified, without
recompiling the program. Even more, it can
be done while it is executing.]]

DEMO:add_stage [[
As an example, this entity shows a textbox
when the player collides with it.]]

DEMO:add_stage [[
That text is in a lua script, and can be
changed easily.]]

DEMO:add_stage [[
That file is executed when the map is
loaded...]]

DEMO:add_stage [[
Lua is used for loading everything. This
removes the need to create custom file
formats for maps, animations, etc.]]

DEMO:add_stage [[
And more, the program creates a Unix socket
that can be used to execute any lua code by
simply writing to it.]]

DEMO:add_stage [[
That is it for this demo. If you liked
nngn, feel free to download it, take a look
at the source code, play around with it,
and even make contributions.]]

DEMO:add_stage "Thanks for watching."

input.input:add(
    Input.KEY_ENTER, Input.SEL_PRESS,
    function() DEMO:next() end)
DEMO:next()
function demo_start() end
