dofile "src/lua/path.lua"
local entity = require "nngn.lib.entity"
local player = require "nngn.lib.player"
local texture = require "nngn.lib.texture"
require "src/lua/input"

require("nngn.lib.graphics").init()

local N = 2 ^ 10
nngn:entities():set_max(N)
nngn:renderers():set_max_sprites(N)
nngn:graphics():resize_textures(5)
nngn:textures():set_max(5)
assert(nngn:textures():load(texture.NNGN))
nngn:animations():set_max(N)
nngn:colliders():set_max_colliders(N)
nngn:renderers():set_max_colliders(N)
nngn:colliders():set_max_collisions(N * N)

player.set{
    "src/lson/crono.lua",
    "src/lson/zelda/link.lua",
    "src/lson/zelda/link_sh.lua",
    "src/lson/zelda/fairy0.lua",
    "src/lson/chocobo.lua",
    "src/lson/null.lua",
}

local rnd = function() return (nngn:math():rand() - .5) * 256 end
for i = 1, N - 1 do
    player.load(entity.load(nil, nil, {pos = {rnd(), rnd(), 0}}), true)
end
function demo_start() end
