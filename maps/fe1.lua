local entity = require "nngn.lib.entity"
local map = require "nngn.lib.map"
local player = require "nngn.lib.player"

local entities = {}
local warp

local function init()
    -- warp
    table.insert(entities, entity.load(nil, nil, {
        pos = {0, -144}, collider = {
            type = Collider.AABB, bb = {-32, -16, 32, 16},
            flags = Collider.TRIGGER}}))
    warp = entities[#entities]
    for _, t in ipairs({
        -- solids
        {{   0,   80}, {-96, -16, 96, 16}},
        {{-112,   16}, {-16, -48, 16, 48}},
        {{ 112,   16}, {-16, -48, 16, 48}},
        {{ -48,  -80}, {-16, -48, 16, 48}},
        {{  48,  -80}, {-16, -48, 16, 48}},
        {{ -80,  -48}, {-16, -16, 16, 16}},
        {{  80,  -48}, {-16, -16, 16, 16}},
    }) do
        local pos, bb = table.unpack(t)
        table.insert(entities, entity.load(nil, nil, {
            pos = pos, collider = {
                type = Collider.AABB,
                bb = bb, m = Math.INFINITY, flags = Collider.SOLID}}))
    end
    player.move_all(0, -288, false)
end

map.map {
    name = "fe1",
    file = "maps/fe1.lua",
    init = init,
    entities = entities,
    on_collision = function(e0, e1)
        if e0 == warp or e1 == warp then map.next("maps/fe0.lua") end
    end,
    tiles = {
        "img/fire_emblem/21.png",
        32, 0, -16, 32, 32, 8, 7, {
             8, 28,   9, 28,  10, 28,   0, 27,   1, 27,  10, 28,  8, 28,  2, 27,
            10, 29,   0, 28,   1, 28,   4, 30,   4, 29,   1, 28,  2, 28,  3, 28,
             8, 30,   2, 30,   9, 30,   4, 31,   8, 31,   1, 30,  2, 30,  3, 30,
             3, 31,   4, 30,   8, 31,   4, 29,   8, 31,   4, 29,  4, 29,  3, 31,
             3, 31,   4, 31,   5, 30,   6, 30,   6, 30,   7, 30,  8, 31,  3, 31,
             3, 31,   4, 31,   5, 31,   6, 31,   6, 31,   7, 31,  8, 31,  3, 31,
             1, 30,   2, 30,   2, 30,   2, 30,   2, 30,   2, 30,  2, 30,  9, 30,
         },
    },
}
