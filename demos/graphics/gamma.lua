dofile "src/lua/path.lua"
local camera = require "nngn.lib.camera"
local entity = require "nngn.lib.entity"

require("nngn.lib.graphics").init()
require "src/lua/limits"
require "src/lua/main"

local N, S <const> = 32, 4
local vec3 <const> = require("nngn.lib.math").vec3
local r, g, b <const> = vec3(1, 0, 0), vec3(0, 1, 0), vec3(0, 0, 1)

camera.set_perspective(true)

for i = 0, N do
    local c = vec3(i / N)
    for _, t in ipairs{{6, c}, {2, r * c}, {-2, g * c}, {-6, b * c}} do
        entity.load(nil, nil, {
            pos = {S * (i - N / 2), t[1], 0},
            renderer = {type = Renderer.CUBE, color = t[2], size = S},
        })
    end
end
function demo_start() end
