require "src/lua/path"
local anim = require "nngn.lib.animation"
local entity = require "nngn.lib.entity"
local input = require "nngn.lib.input"

require("nngn.lib.graphics").init()
require("nngn.lib.input").install()

local DIV <const> = 48
nngn.entities:set_max(4)
nngn.renderers:set_sphere_divisions(DIV, DIV)
nngn.renderers:set_max_spheres(1)

nngn.renderers:set_perspective(true)
nngn.camera:set_perspective(true)
nngn.camera:set_pos(0, -128, 128)
nngn.camera:set_rot(math.pi / 2, 0, 0)
nngn.lighting:set_ambient_light(1, 1, 1, 0.25)

entity.load(nil, nil, {
    pos = {0, 0, 128},
    renderer = {type = Renderer.SPHERE, r = 64},
})

local lights = {}
do
    local d = 128
    for _, t in ipairs{
        {{1, 0, 0, 1}, math.pi / -4},
        {{0, 1, 0, 1}, math.pi / 6},
        {{0, 0, 1, 1}, math.pi * 5 / 6},
    } do
        table.insert(lights, entity.load(nil, nil, {
            pos = {128 * math.cos(t[2]), 128 * math.sin(t[2]), 64},
            light = {type = Light.POINT, color = t[1], att = 2048, spec = 0},
        }))
    end
end

local a = anim.camera_rev({0, 0, 0}, 0.001)
local sphere_div = 0
local continuous = false

local function step()
    sphere_div = sphere_div % DIV + 2
    nngn.renderers:set_sphere_divisions(sphere_div, sphere_div)
end

input.input:remove(string.byte("N"))
input.input:add(string.byte("N"), Input.SEL_PRESS, function()
    nngn.renderers:set_sphere_interp_normals(
        not nngn.renderers:sphere_interp_normals())
end)

input.input:remove(string.byte(" "))
input.input:add(string.byte(" "), 0, function(_, press, mods)
    local shift <const> = mods & Input.MOD_SHIFT ~= 0
    continuous = press and shift
    if press or (shift and sphere_div ~= DIV) then
        step()
    end
end)

nngn.schedule:next(Schedule.HEARTBEAT, function()
    if continuous and sphere_div ~= DIV then step() end
    a:update(nngn.timing:fdt_ms())
    local now = nngn.timing:fnow_ms()
    local z = 128 + 128 * math.sin(now / 512)
    for _, x in ipairs(lights) do
        local pos = {x:pos()}
        pos[3] = z
        x:set_pos(table.unpack(pos))
    end
end)
