local camera = require "nngn.lib.camera"
local entity = require "nngn.lib.entity"
local font = require "nngn.lib.font"
local player = require "nngn.lib.player"
local texture = require "nngn.lib.texture"
dofile "src/lua/input.lua"

camera.reset()
player.set{
    "src/lson/crono.lua", "src/lson/link.lua", "src/lson/link_sh.lua",
    "src/lson/fairy0.lua", "src/lson/chocobo.lua", "src/lson/null.lua",
}
nngn.textures:load(texture.NNGN)
font.load()
nngn.grid:set_dimensions(32.0, 64)
entities = {
    entity.load(nil, "src/lson/old_man.lua", {pos = {-32, 0}}),
    entity.load(nil, nil, {
        pos = {32, 0, 8},
        renderer = {type = Renderer.CUBE, size = 16, color = {1, .5, 0}},
        collider = {
            type = Collider.AABB, flags = Collider.SOLID,
            bb = {-8, -8, 8, 8}, m = 1/0,
        }
    }),
    entity.load(nil, nil, {
        pos = {64, 0, 8},
        renderer = {
            type = Renderer.VOXEL, size = {16, 16, 16}, tex = "img/zelda.png",
            uv = {
                0, 1, 0x1p-4, 1 - 0x1p-4,
                0, 1, 0x1p-4, 1 - 0x1p-4,
                0, 1, 0x1p-4, 1 - 0x1p-4,
                0, 1, 0x1p-4, 1 - 0x1p-4,
                0, 1, 0x1p-4, 1 - 0x1p-4,
                0, 1, 0x1p-4, 1 - 0x1p-4,
            },
        },
        collider = {
            type = Collider.AABB, flags = Collider.SOLID,
            bb = {-8, -8, 8, 8}, m = 1/0,
        },
    }),
    entity.load(nil, "src/lson/fairy0.lua", {pos = {96, 0}}),
    entity.load(nil, "src/lson/rock0.lua", {pos = {128, 16}}),
    entity.load(nil, "src/lson/tree0.lua", {pos = {192, 32}}),
}
