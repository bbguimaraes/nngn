dofile "src/lua/path.lua"
local entity = require "nngn.lib.entity"
local texture = require "nngn.lib.texture"
require "src/lua/input"

require("nngn.lib.graphics").init()

local N <const> = 2 ^ 10
nngn:entities():set_max(N + 1)
nngn:renderers():set_max_sprites(N)
nngn:graphics():resize_textures(3)
nngn:textures():set_max(3)

function rnd(w) return (nngn:math():rand() - .5) * 512 end
function rnd_pos() return {rnd(), rnd(), 0} end

local tex <close> = texture.load("img/zelda/zelda.png")
local renderers = {{
    type = Renderer.SPRITE, size = {24, 24}, tex = tex.tex,
    scale = {512//32, 512//32},
}, {
    type = Renderer.SPRITE, size = {16, 16}, tex = tex.tex,
    scale = {512//16, 512//16}, coords = {2, 0},
}}
local n_renderers <const> = #renderers

for i = 1, N do
    local r = nngn:math():rand_int(n_renderers)
    entity.load(nil, nil, {
        pos = rnd_pos(),
        renderer = renderers[r],
    })
end

function demo_start() end
