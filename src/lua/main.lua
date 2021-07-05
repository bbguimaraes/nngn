local entity = require "nngn.lib.entity"
dofile "src/lua/input.lua"

entities = {
    entity.load(nil, "src/lson/old_man.lua", {pos = {-32, 0}}),
    entity.load(nil, nil, {
        pos = {32, 0, 8},
        renderer = {type = Renderer.CUBE, size = 16, color = {1, .5, 0}}}),
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
    }),
}
