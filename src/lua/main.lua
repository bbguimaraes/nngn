local camera = require "nngn.lib.camera"
local entity = require "nngn.lib.entity"
local font = require "nngn.lib.font"
local map = require "nngn.lib.map"
local math = require "nngn.lib.math"
local player = require "nngn.lib.player"
local texture = require "nngn.lib.texture"
dofile "src/lua/input.lua"

nngn.input:register_callback(map.key_callback)

function on_collision(e0, e1, ...)
    map.on_collision(e0, e1, ...)
    local fire = player.data(nil, "fire")
    if fire then
        local e = fire.entity
        if e and (e0 == e or e1 == e) then fire.remove() end
    end
end

camera.reset(2)
player.set{
    "src/lson/crono.lua", "src/lson/link.lua", "src/lson/link_sh.lua",
    "src/lson/fairy0.lua", "src/lson/chocobo.lua", "src/lson/null.lua"}
nngn.textures:load(texture.NNGN)
font.load(32)
nngn.grid:set_dimensions(32.0, 64)

local rand = function() return nngn.math:rand() end
local tracer = nngn.tracer
local n = 500
local spread = 11
tracer:set_enabled(true)
tracer:set_n_threads(4)
tracer:set_min_t(0.001)
tracer:set_max_t(9999999)
tracer:set_max_samples(1024)
tracer:set_max_lambertians(n + 2);
tracer:set_max_metals(n + 1);
tracer:set_max_dielectrics(n + 1);
for a = -spread, spread - 1 do
    for b = -spread, spread - 1 do
        local center = math.vec3(a + .9 * rand(), .2, b + .9 * rand())
        if (center - math.vec3(4, .2, 0)):len() <= .9 then goto continue end
        local choice = rand()
        if choice < .8 then
            tracer:add_sphere(
                center, .2,
                tracer:add_lambertian(
                    math.vec3_rand(rand) * math.vec3_rand(rand)))
        elseif choice < .95 then
            tracer:add_sphere(
                center, .2,
                tracer:add_metal(
                    (math.vec3_rand(rand) + math.vec3(1)) * math.vec3(0.5),
                    0.5, rand()))
        else
            tracer:add_sphere(center, .2, tracer:add_dielectric(1.5))
        end
        ::continue::
    end
end
tracer:add_sphere({0, 1, 0}, 1, tracer:add_dielectric(1.5))
tracer:add_sphere({-4, 1, 0}, 1, tracer:add_lambertian({.4, .2, .1}))
tracer:add_sphere({4, 1, 0}, 1, tracer:add_metal({.7, .6, .5}, 0))
tracer:add_sphere({0, -1000, 0}, 1000, tracer:add_lambertian(math.vec3(.5)))

local c = tracer:camera()
camera.set(c)
camera.reset(1)
camera.set_max_vel(2)
c:look_at(0, 0, 0, 13, 2, 3, 0, 1, 0)

local tex = {}
for _ = 1, 4 * 512 * 512 do table.insert(tex, 0) end
entity.load(nil, nil, {
    renderer = {
        type = Renderer.SPRITE,
        tex = nngn.textures:load_data("lua:ray", tex),
        size = {512, 512}, scale = {1, 1}}})
