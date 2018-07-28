local entity = require "nngn.lib.entity"
local map = require "nngn.lib.map"
local player = require "nngn.lib.player"
local state = require "nngn.lib.state"
local texture = require "nngn.lib.texture"

local entities = {}
local warps = {}
local tex <close> = texture.load("img/chrono_trigger/house.png")
local data = map.data() or state.save({ambient_light = true})

local function init()
    -- warps
    for _, t in pairs({
        {{48, -120}, {-16, -24, 16, 8}, {
            type = Renderer.SPRITE,
            tex = tex.tex, size = {32, 32},
            scale = {512//32, 512//32}, coords = {0, 2}}},
        {{ 16, 58}, {16, 24}},
        {{-24, 56}, {16, 32}},
        {{-72, 56}, {16, 32}},
    }) do
        local e = entity.load(nil, nil, {
            pos = t[1], renderer = t[3], collider = {
                type = Collider.AABB, bb = t[2],
                flags = Collider.TRIGGER}})
        table.insert(entities, e)
        table.insert(warps, e)
    end
    -- solids
    for _, t in ipairs({
        -- walls
        {{ -88,   48}, {          16, 32}},
        {{ -48,   48}, {          32, 32}},
        --{{ -56,   48}, {-40, -16, 40, 16}},
        {{  80,   48}, {-16, -16, 16, 16}},
        {{-112,  -32}, {-16, -64, 16, 64}},
        {{ 112,  -32}, {-16, -64, 16, 64}},
        {{  24,  -16}, {-24, -32, 24, 32}},
        {{ -32, -112}, {-64, -16, 64, 16}},
        {{  80, -112}, {-16, -16, 16, 16}},
        -- vase
        {{  -8,   24}, { -8,  -8,  8,  8}},
        -- stairs
        {{  56,   16}, { -8, -16,  8, -8}},
    }) do
        table.insert(entities, entity.load(nil, nil, {
            pos = t[1], collider = {
                type = Collider.AABB,
                bb = t[2], m = Math.INFINITY, flags = Collider.SOLID}}))
    end
    for _, t in ipairs({
        {{  36,   22}, { -4, -26,  4, 26}, 45},
        {{  46, 56.5}, { -4, -30,  4, 30}, 45},
    }) do
        table.insert(entities, entity.load(nil, nil, {
            pos = t[1], collider = {
                type = Collider.BB, bb = t[2], rot = math.rad(t[3]),
                m = Math.INFINITY, flags = Collider.SOLID}}))
    end
    for _, t in ipairs({
        -- table
        {{-48, -32}, {-16, -32, 16,  24}, {0, 0}, {32, 64}, {0, 0}},
        -- chairs
        {{-88, -16}, { -8, -16,  8,  -8}, {0, 0}, {16, 32}, {3, 0}},
        {{-56,  16}, { -8, -16,  8,  -8}, {0, 0}, {16, 32}, {2, 0}},
    }) do
        table.insert(entities, entity.load(nil, nil, {
            pos = t[1],
            renderer = {
                type = Renderer.SPRITE,
                tex = tex.tex, size = t[4],
                scale = {512//t[4][1], 512//t[4][2]}, coords = t[5]},
            collider = {
                type = Collider.AABB,
                bb = t[2], m = Math.INFINITY, flags = Collider.SOLID}}))
    end
    -- stairs
    table.insert(entities, entity.load(nil, nil, {
        pos = {40, 24}, renderer = {
            type = Renderer.SPRITE,
            tex = tex.tex, size = {48, 48},
            scale = {512//16, 512//16}, coords = {7, 0, 10, 3}}}))
    -- south wall
    table.insert(entities, entity.load(nil, nil, {
        pos = {0, -88}, renderer = {
            type = Renderer.SPRITE,
            tex = tex.tex, size = {256, 32}, z_off = -32,
            scale = {512//256, 512//32}, coords = {0, 2}}}))
    if map.name() == "house1" then
        player.move_all(64, 128, false)
        local _, _, _, a = nngn.lighting:ambient_light()
        nngn.lighting:set_ambient_anim({
            rate_ms = 0, f = {
                type = AnimationFunction.LINEAR,
                v = a, step_s = 5,
                ["end"] = map.data().ambient_light[4]}})
    else
        player.move_all(72, -104, false)
    end
end

local function on_collision(e0, e1)
    if e0 == warps[1] or e1 == warps[1] then
        state.restore(map.data())
        map.next("maps/main.lua")
    elseif e0 == warps[2] or e1 == warps[2] then
        map.next("maps/house1.lua")
    elseif e0 == warps[3] or e1 == warps[3] then
        map.next("maps/cathedral.lua")
    elseif e0 == warps[4] or e1 == warps[4] then
        map.next("maps/fiona.lua")
    end
end

map.map {
    name = "house0",
    file = "maps/house0.lua",
    init = init,
    entities = entities,
    on_collision = on_collision,
    data = data,
    tiles = {tex.tex, 2, 0, 0, 256, 256, 1, 1, {0, 1}},
}
