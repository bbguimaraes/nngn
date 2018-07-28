local entity = require "nngn.lib.entity"
local map = require "nngn.lib.map"
local player = require "nngn.lib.player"

local entities = {}
local warp

local function init()
    -- warp
    table.insert(entities, entity.load(nil, nil, {
        pos = {-112,  16}, collider = {
            type = Collider.AABB, bb = {-16, -32, 16, 32},
            flags = Collider.TRIGGER}}))
    warp = entities[#entities]
    for _, t in ipairs({
        -- solids
        {{ -16,  64}, {-80, -16, 80, 16}},
        {{  80,  24}, {-16, -24, 16, 24}},
        {{  48, -16}, {-16, -16, 16, 16}},
        {{ -72, -24}, {-24,  -8, 24,  8}},
        {{ -60, -40}, { -4,  -8,  4,  8}},
        {{ -32, -64}, {-24, -16, 24, 16}},
        {{  12, -40}, {-20,  -8, 20,  8}},
        {{  -8, -24}, { -8,  -8,  8,  8}},
    }) do
        local pos, bb = table.unpack(t)
        table.insert(entities, entity.load(nil, nil, {
            pos = pos, collider = {
                type = Collider.AABB,
                bb = bb, m = Math.INFINITY, flags = Collider.SOLID}}))
    end
    for _, t in ipairs({
        {"src/lson/cow.lua",     {-64, 34}},
        {"src/lson/horse.lua",   {-32, 36}},
        {"src/lson/dog.lua",     { -8, 34}},
        {"src/lson/chicken.lua", { 17, 32}},
        {"src/lson/bird.lua",    { 40, 32}},
    }) do
        local f, pos = table.unpack(t)
        local t = dofile(f)
        t.pos = pos
        t.collider.flags = Collider.SOLID
        t.collider.m = Math.INFINITY
        table.insert(entities, entity.load(nil, nil, t))
    end
    player.move_all(-36, 32, false)
end

map.map {
    name = "hm",
    file = "maps/hm.lua",
    init = init,
    entities = entities,
    on_collision = function(e0, e1)
        if e0 == warp or e1 == warp then map.next("maps/main.lua") end
    end,
    tiles = {
        "img/harvest_moon.png",
        32, 0, 0, 16, 16, 12, 8, {
            0, 0,  5, 3,  0, 5,  0, 5,  0, 5,  0, 5,  6, 4,  6, 2,  0, 2,  4, 2,  5, 2,  5, 2,
            0, 1,  5, 4,  1, 0,  2, 0,  3, 0,  4, 0,  3, 5,  3, 5,  3, 5,  3, 5,  0, 5,  0, 4,
            0, 5,  3, 5,  0, 5,  2, 3,  2, 3,  2, 2,  2, 1,  2, 1,  5, 0,  6, 0,  5, 5,  6, 3,
            5, 3,  1, 1,  2, 1,  2, 3,  2, 3,  2, 3,  2, 3,  2, 3,  5, 1,  6, 1,  5, 5,  0, 3,
            5, 3,  1, 2,  2, 3,  2, 3,  2, 3,  2, 3,  2, 3,  2, 3,  2, 3,  3, 2,  5, 5,  6, 3,
            5, 3,  1, 3,  2, 3,  2, 3,  2, 3,  2, 3,  2, 3,  2, 3,  2, 3,  3, 3,  5, 5,  6, 4,
            5, 3,  1, 4,  2, 4,  2, 4,  2, 4,  2, 4,  2, 4,  2, 4,  2, 4,  3, 4,  5, 5,  6, 5,
            0, 5,  1, 5,  2, 5,  2, 5,  2, 5,  2, 5,  2, 5,  2, 5,  2, 5,  4, 5,  5, 5,  6, 5,
        },
    },
}
