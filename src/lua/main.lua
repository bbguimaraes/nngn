local camera = require "nngn.lib.camera"
local entity = require "nngn.lib.entity"
dofile "src/lua/input.lua"

camera.reset()
camera.set_fov_z()
entities = {
    entity.load(nil, "src/lson/old_man.lua", {pos = {-32, 0}}),
    entity.load(nil, nil, {
        pos = {32, 0, 8},
        renderer = {type = Renderer.CUBE, size = 16, color = {1, .5, 0}}}),
}
