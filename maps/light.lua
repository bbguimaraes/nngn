local camera = require "nngn.lib.camera"
local entity = require "nngn.lib.entity"
local map = require "nngn.lib.map"
local player = require "nngn.lib.player"
local texture = require "nngn.lib.texture"

local state = {
    perspective = true, ambient_light = true, shadows_enabled = true}
local entities = {}
local warp, sun_dir, sun

local function init()
    -- warp
    table.insert(entities, entity.load(nil, nil, {
        collider = {
            type = Collider.AABB, bb = 32, flags = Collider.TRIGGER}}))
    warp = entities[#entities]
    -- solids
    for _, t in ipairs({
        {{  0,  528}, {-512, -16, 512, 16}},
        {{-528,   0}, {-16, -512, 16, 512}},
        {{ 528,   0}, {-16, -512, 16, 512}},
        {{  0, -528}, {-512, -16, 512, 16}},
    }) do
        table.insert(entities, entity.load(nil, nil, {
            pos = t[1], collider = {
                type = Collider.AABB,
                bb = t[2], m = Math.INFINITY, flags = Collider.SOLID}}))
    end
    -- cubes
    table.insert(entities, entity.load(nil, nil, {
        pos = {0, 0, 16}, renderer = {type = Renderer.CUBE, size = 32}}))
    table.insert(entities, entity.load(nil, nil, {
        pos = {0, -48, 48}, renderer = {type = Renderer.CUBE, size = 8}}))
    -- sun
    table.insert(entities, entity.load(nil, nil, {
        renderer = {type = Renderer.CUBE, size = 8}}))
    sun_dir = entities[#entities]
    -- old man
    do
        local t = dofile("src/lson/old_man.lua")
        t.pos = {0, -32, 0}
        t.collider.flags = Collider.TRIGGER | Collider.SOLID
        t.collider.m = Math.INFINITY
        table.insert(entities, entity.load(nil, nil, t))
    end
    -- lights
    sun = nngn:lighting():sun()
    sun:set_incidence(math.rad(315))
    sun:set_time_ms(6 * 1000 * 60 * 60)
    local sun_light = nngn:lighting():add_light(Light.DIR)
    if sun_light then
        sun_light:set_color(1, 0.8, 0.5, 1)
        nngn:lighting():set_sun_light(sun_light)
    end
    table.insert(entities, entity.load(nil, nil, {
        light = {
            type = Light.DIR,
            color = {0.5, 0.25, 0, 1}, dir = {0, 1, 0}}}))
    do
        local colors = {
            {1, 0, 0, 1}, {0, 1, 0, 1}, {0, 0, 1, 1}, {1, 0, 1, 1}};
        for i = 0, 3 do
            table.insert(entities, entity.load(nil, nil, {
                pos = {i % 2 * 128 - 64, i // 2 * 128 - 64, 16},
                light = {
                    type = Light.POINT,
                    color = colors[i + 1], att = 512}}))
        end
    end
    player.move_all(8, -8, false)
    nngn:camera():set_perspective(true)
    nngn:renderers():set_perspective(true)
    camera.reset(4)
    nngn:lighting():set_ambient_light(0.15, 0.15, 0.15, 1)
    nngn:lighting():set_shadows_enabled(true)
end

local function heartbeat()
    local v = {sun:dir()}
    for i = 1, 3 do v[i] = v[i] * -128 end
    nngn.entities:set_pos(sun_dir, table.unpack(v))
end

map.map {
    name = "light",
    file = "maps/light.lua",
    state = state,
    init = init,
    entities = entities,
    heartbeat = heartbeat,
    on_collision = function(e0, e1)
        if e0 == warp or e1 == warp then map.next("maps/main.lua") end
    end,
    reset = function()
        camera.reset()
        local l = nngn:lighting():sun_light()
        if l then
            nngn:lighting():remove_light(l)
            nngn:lighting():set_sun_light(nil)
        end
    end,
    tiles = {texture.NNGN, 512, 0, 0, 1024, 1024, 1, 1, {0, 0}},
}
