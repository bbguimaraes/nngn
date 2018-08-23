dofile "src/lua/path.lua"
local camera = require "nngn.lib.camera"
local entity = require "nngn.lib.entity"
local texture = require "nngn.lib.texture"
require "src/lua/input"

require("nngn.lib.graphics").init()
camera.reset()

local N = 2 ^ 10
nngn.entities:set_max(N + 1)
nngn.animations:set_max(N)
nngn.renderers:set_max_sprites(N)
nngn.renderers:set_max_cubes(N)
nngn.graphics:resize_textures(3)
nngn.textures:set_max(3)

local rnd = function(w) return (nngn.math:rand() - .5) * 512 end
local rnd_pos = function() return {rnd(), rnd(), 0} end
local tex <close> = texture.load("img/zelda.png")
local fairy0 = dofile("src/lson/fairy0.lua")
fairy0.tex = tex.tex
local renderers = {{
    type = Renderer.SPRITE, size = {24, 24}, tex = tex.tex,
    scale = {512//32, 512//32},
}, {
    type = Renderer.SPRITE, size = {16, 16}, tex = tex.tex,
    scale = {512//16, 512//16}, coords = {2, 0},
}, {
    type = Renderer.CUBE, size = 8, color = {1, 1, 1},
}, fairy0.renderer}
local n_renderers = #renderers
local anim = nngn.animations:load(fairy0.anim)
local anims = {nil, nil, nil, {sprite = anim:sprite()}}
for i = 1, N do
    local r = nngn.math:rand_int(n_renderers)
    entity.load(nil, nil, {
        pos = rnd_pos(), renderer = renderers[r], anim = anims[r]})
end

nngn.animations:remove(anim)
nngn.renderers:set_zsprites(true)
nngn.lighting:set_zsprites(true)
nngn.lighting:set_shadows_enabled(true)
nngn.lighting:set_ambient_light(.25, .25, .25, 1)
entity.load(nil, nil, {
    pos = {0, 0, 32}, light = {
        type = Light.POINT, color = {1, .8, .5, 1}, att = 512}})
