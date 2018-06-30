local camera = require "nngn.lib.camera"
local entity = require "nngn.lib.entity"
local font = require "nngn.lib.font"
local player = require "nngn.lib.player"
dofile "src/lua/input.lua"

camera.reset()
camera.set_fov_z()
player.set("src/lson/crono.lua")
font.load()
entities = {
    entity.load(nil, "src/lson/old_man.lua", {pos = {-32, 0}}),
    entity.load(nil, nil, {
        pos = {32, 0, 8},
        renderer = {type = Renderer.CUBE, size = 16, color = {1, .5, 0}}}),
    entity.load(nil, nil, {
        pos = {64, 0, 8},
        renderer = {
            type = Renderer.VOXEL,
            tex = "img/zelda/zelda.png",
            size = {16, 16, 16},
            uv = {
                0, 1, 0x1p-4, 1 - 0x1p-4,
                0, 1, 0x1p-4, 1 - 0x1p-4,
                0, 1, 0x1p-4, 1 - 0x1p-4,
                0, 1, 0x1p-4, 1 - 0x1p-4,
                0, 1, 0x1p-4, 1 - 0x1p-4,
                0, 1, 0x1p-4, 1 - 0x1p-4,
            },
        },
    }),
    entity.load(nil, "src/lson/zelda/fairy0.lua", {pos = {96, 0}}),
}
