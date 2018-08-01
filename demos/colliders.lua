dofile "src/lua/path.lua"
local entity <const> = require "nngn.lib.entity"

require("nngn.lib.graphics").init()
require("nngn.lib.player").set{"src/lson/null.lua"}
require("src/lua/input")

local N <const> = 2 ^ 11
nngn.entities:set_max(N)
nngn.graphics:resize_textures(2)
nngn.textures:set_max(2)
assert(nngn.textures:load(require("nngn.lib.texture").NNGN))
nngn.colliders:set_max_colliders(N)
nngn.renderers:set_max_colliders(N)
nngn.colliders:set_max_collisions(32 * N)
nngn.renderers:set_debug(Renderers.DEBUG_BB | Renderers.DEBUG_CIRCLE)

local C <const> = Collider
local colliders <const> = {
    {type = C.AABB, bb = 8},
}
local n <const> = #colliders
local math <const> = nngn.math
local rnd <const> = (function()
    local rnd = math.rand
    return function() return (rnd(math) - .5) * 256 end
end)()

for i = 1, N do
    entity.load(nil, nil, {
        pos = {rnd(), rnd(), 0},
        collider = colliders[math:rand_int(1, n)]})
end
