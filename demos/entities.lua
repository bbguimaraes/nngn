dofile "src/lua/path.lua"
local entity = require "nngn.lib.entity"
require "src/lua/input"

require("nngn.lib.graphics").init()

local N = 2 ^ 10
nngn.entities:set_max(N + 1)
nngn.renderers:set_max_sprites(N)

local rnd = function(w) return (nngn.math:rand() - .5) * 512 end
local rnd_pos = function() return {rnd(), rnd(), 0} end
local renderers = {{
    type = Renderer.SPRITE, size = {8, 8}}}
local n_renderers = #renderers
for i = 1, N do
    local r = nngn.math:rand_int(n_renderers)
    entity.load(nil, nil, {
        pos = rnd_pos(), renderer = renderers[r]})
end
