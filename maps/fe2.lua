local entity = require "nngn.lib.entity"
local map = require "nngn.lib.map"
local player = require "nngn.lib.player"

local entities = {}
local warp

local function init()
    -- warp
    table.insert(entities, entity.load(nil, nil, {
        pos = {-16, 104}, collider = {
            type = Collider.AABB, bb = {-16,  -8, 16,  8},
            flags = Collider.TRIGGER}}))
    warp = entities[#entities]
    for _, t in ipairs({
        -- solids
        {{-48,  80}, {-16, -16, 16, 16}},
        {{ 32,  80}, {-32, -16, 32, 16}},
        {{-80,  16}, {-16, -48, 16, 48}},
        {{ 80,  16}, {-16, -48, 16, 48}},
        {{  0, -48}, {-64, -16, 64, 16}},
    }) do
        table.insert(entities, entity.load(nil, nil, {
            pos = t[1], collider = {
                type = Collider.AABB,
                bb = t[2], m = Math.INFINITY, flags = Collider.SOLID}}))
    end
    -- chests
    for _, p in ipairs({{16, -16}, {-16, 16}}) do
        table.insert(entities, entity.load(nil, nil, {
            pos = p,
            renderer = {
                type = Renderer.SPRITE,
                tex = "img/fire_emblem/21.png", size = {32, 32},
                scale = {512//16, 512//16}, coords = {1, 23, 2, 24}},
            collider = {
                type = Collider.AABB,
                bb ={-16, -16, 16, 0}, m = Math.INFINITY,
                flags = Collider.SOLID}}))
    end
    player.move_all(192, 224, false)
end

map.map {
    name = "fe2",
    file = "maps/fe2.lua",
    init = init,
    entities = entities,
    on_collision = function(e0, e1)
        if e0 == warp or e1 == warp then map.next("maps/fe0.lua") end
    end,
    tiles = {
        "img/fire_emblem/21.png",
        32, 0, 0, 32, 32, 6, 6, {
             5, 27,   5, 27,  5, 27,   5, 27,   5, 27,  5, 27,
             8, 30,   2, 30,  2, 30,   2, 30,   2, 30,  3, 30,
             0, 23,  10, 27,  9, 28,   1, 23,  10, 27,  3, 31,
             0, 23,  10, 27,  1, 23,  10, 27,   1, 26,  3, 31,
             3, 31,   8, 28,  9, 28,   9, 28,   2, 27,  3, 31,
             5, 26,   6, 26,  7, 26,   8, 26,   2, 30,  9, 30,
         },
     },
}
