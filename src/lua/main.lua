local camera = require "nngn.lib.camera"
local entity = require "nngn.lib.entity"
dofile "src/lua/input.lua"

camera.reset()
camera.set_fov_z()
entities = {
    entity.load(nil, "src/lson/old_man.lua", {pos = {-32, 0}}),
}
