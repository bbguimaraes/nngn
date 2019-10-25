local camera = require "nngn.lib.camera"
local entity = require "nngn.lib.entity"
local light = require "nngn.lib.light"
local map = require "nngn.lib.map"

local state = {perspective = true, ambient_light = true, shadows_enabled = true}
local entities = {}
local map_size = 64
local tiles = {}
for i = 1, map_size * map_size do
    table.insert(tiles, 2)
    table.insert(tiles, 26)
end

local function init()
    -- models
    for _, t in ipairs({
        {"src/lson/coloseum.lua", {  0,    0,   0}, .03},
        {"src/lson/ionic.lua",    {224, -224,   0}, 1},
        {"src/lson/crate.lua",    {-96, -272,   0}, 8},
        {"src/lson/moon.lua",     {  0,    0, 256}, 1},
        {"src/lson/italy.lua",    {  0, -416,   0}, {20, 20, 100}},
    }) do
        local f, p, s = table.unpack(t)
        local t = dofile(f)
        t.pos = p
        t.renderer.scale = s
        table.insert(entities, entity.load(nil, nil, t))
    end
    local pos_filter = function(x, y)
        return x * x + y * y < 320 * 320 or (math.abs(x) < 128 and y < -256)
    end
    do
        local t = dofile("src/lson/tree0.lua")
        -- trees
        for _, p in ipairs(vec2_random_dist(128, 0, 0, 896, 896)) do
            if not pos_filter(table.unpack(p)) then
                t.pos = p
                table.insert(entities, entity.load(nil, nil, t))
            end
        end
        -- tree groups
        for _, c in ipairs({{-256, -352}, {288, 256}}) do
            for _, p in
                ipairs(vec2_random_dist(16, c[1], c[2], 256, 128))
            do
                t.pos = p
                table.insert(entities, entity.load(nil, nil, t))
            end
        end
    end
    do
        local t = dofile("src/lson/rock0.lua")
        -- rocks
        for _, p in ipairs(vec2_random_dist(128, 0, 0, 896, 896)) do
            if not pos_filter(table.unpack(p)) then
                t.pos = p
                table.insert(entities, entity.load(nil, nil, t))
            end
        end
        -- rock groups
        for _, c in ipairs({{-256, -400}, {288, -304}}) do
            for _, p in
                ipairs(vec2_random_dist(8, c[1], c[2], 256, 32))
            do
                t.pos = p
                table.insert(entities, entity.load(nil, nil, t))
            end
        end
    end
    nngn:camera():set_perspective(true)
    nngn:renderers():set_perspective(true)
    camera.reset(1)
    nngn:lighting():set_ambient_light(0.15, 0.15, 0.15, 1)
    nngn:lighting():set_shadows_enabled(true)
    nngn:lighting():set_shadow_map_far(512)
    light.sun(true)
end

map.map {
    name = "colosseum",
    state = state,
    init = init,
    entities = entities,
    tiles = {
        nngn:textures():load("img/fire_emblem/21.png"),
        32, 0, 0, 16, 16, map_size, map_size, tiles,
    },
}
