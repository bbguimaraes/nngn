local entity = require "nngn.lib.entity"
local map = require "nngn.lib.map"
local player = require "nngn.lib.player"
local texture = require "nngn.lib.texture"

local entities = {}
local warp, torch
local tex <close> = texture.load("img/chrono_trigger/fionas_forest.png")

local function init()
    -- warp
    table.insert(entities, entity.load(nil, nil, {
        pos = {144, -88}, collider = {
            type = Collider.AABB, bb = {-16, -32, 16, 32},
            flags = Collider.TRIGGER}}))
    warp = entities[#entities]
    -- solids
    for _, t in ipairs({
        {{ -66,  32}, {-30, -16, 30, 16}},
        {{ -16,  48}, {-20, -16, 20, 16}},
        {{  20,  32}, {-16, -16, 16, 16}},
        {{  74,  48}, {-38, -16, 38, 16}},
        {{-112, -20}, {-16, -36, 16, 36}},
        {{-128, -96}, {-16, -24, 16, 24}},
        {{ 128,  -4}, {-16, -36, 16, 36}},
    }) do
        table.insert(entities, entity.load(nil, nil, {
            pos = t[1], collider = {
                type = Collider.AABB,
                bb = t[2], m = Math.INFINITY, flags = Collider.SOLID}}))
    end
    for _, t in ipairs({
        -- background trees
        {{  0,  64}, nil, {256, 128},  -48, {256, 128}, {0, 3, 1, 4}},
        {{  0,  72}, nil, {256,  64},  -24, {256,  64}, {0, 4, 1, 5}},
        {{  0,  64}, nil, {256,  64},  nil, {256,  64}, {0, 5, 1, 6}},
        -- trees
        {{  0, -96}, {-112, -56, 128, -24}, {256,  64}, -40, {256,  64},
         { 1, 0}},
        {{-80, -16}, { -32, -56,   8, -40}, { 96, 160}, -58, { 32,  32},
         { 8, 2, 11, 7}},
        {{ 96,  16}, {  -8, -72,  32, -56}, { 64, 160}, -74, { 32,  32},
         {14, 2, 16, 7}},
    }) do
        table.insert(entities, entity.load(nil, nil, {
            pos = t[1],
            renderer = {
                type = Renderer.SPRITE,
                tex = tex.tex, size = t[3], z_off = t[4],
                scale = {512//t[5][1], 512//t[5][2]}, coords = t[6]},
            collider = {
                type = Collider.AABB,
                bb = t[2], m = Math.INFINITY, flags = Collider.SOLID}}))
    end
    -- campfire
    do
        local t = dofile("src/lson/campfire.lua")
        t.pos = {0, -24}
        t.collider.flags = (t.collider.flags or 0) | Collider.SOLID
        t.collider.m = Math.INFINITY
        table.insert(entities, entity.load(nil, nil, t))
        local e = entities[#entities]
        table.insert(entities, entity.load(nil, nil, {
            pos = {0, -12, 16}, parent = e,
            light = {
                type = Light.POINT,
                color = {1, 1, .7, 1}, att = 512, spec = 0}}))
    end
    -- hero's grave torch
    table.insert(entities, entity.load(nil, nil, {pos = {48, 20}}))
    do
        local c = dofile("src/lson/heros_grave_torch.lua").collider
        c.flags = Collider.TRIGGER
        entities[#entities]:set_collider(nngn:colliders():load(c))
    end
    torch = entities[#entities]
    nngn:lighting():set_shadows_enabled(true)
    nngn:lighting():set_zsprites(true)
    nngn:renderers():set_zsprites(true)
    nngn:lighting():set_ambient_anim({
        timer_ms = 120, rate_ms = 120,
        f = {type = AnimationFunction.RANDOM_F, min = .1, max = .1625}})
    player.move_all(112, -80, true)
end

local function on_collision(e0, e1)
    if e0 == warp or e1 == warp then
        map.next("maps/main.lua")
    elseif e0 == torch or e1 == torch then
        local t = dofile("src/lson/heros_grave_torch.lua")
        t.collider.flags = (t.collider.flags or 0) | Collider.SOLID
        t.collider.m = Math.INFINITY
        entity.load(torch, nil, t)
        table.insert(entities, entity.load(nil, nil, {
            pos = {0, -8, 32}, parent = torch,
            light = {
                type = Light.POINT,
                color = {.753, .973, .973, 1}, att = 512, spec = 0}}))
    end
end

map.map {
    name = "fiona",
    file = "maps/fiona.lua",
    state = {ambient_light = true, shadows_enabled = true, zsprites = true},
    init = init,
    entities = entities,
    on_collision = on_collision,
    reset = function() nngn:lighting():set_ambient_anim({f = {}}) end,
    tiles = {tex.tex, 2, 0, 0, 256, 256, 1, 1, {0, 0}},
}
