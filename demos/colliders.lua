dofile "src/lua/path.lua"
local entity = require "nngn.lib.entity"
local player = require "nngn.lib.player"
local texture = require "nngn.lib.texture"
require "src/lua/input"

player.set{"src/lson/null.lua"}

require("nngn.lib.graphics").init()

local N = 2 ^ 12
nngn.entities:set_max(N)
nngn.graphics:resize_textures(2)
nngn.textures:set_max(2)
assert(nngn.textures:load(texture.NNGN))
nngn.colliders:set_max_colliders(N)
nngn.renderers:set_max_colliders(N)
nngn.colliders:set_max_collisions(32 * N)
local colliders = {
--    {type = Collider.AABB, flags = Collider.SOLID, bb = 8},
--    {type = Collider.BB,
--     flags = Collider.SOLID, bb = 8, rot = nngn.math:rand()},
    {type = Collider.SPHERE, flags = Collider.SOLID, r = 4}}
local n_col = #colliders
local rnd = function() return (nngn.math:rand() - .5) * 512 end
for i = 1, N do
    entity.load(nil, nil, {
        pos = {rnd(), rnd(), 0},
        collider = colliders[nngn.math:rand_int(1, n_col)]})
end

nngn.renderers:set_debug(Renderers.BB | Renderers.CIRCLE)
