local entity = require "nngn.lib.entity"
local map = require "nngn.lib.map"
local player = require "nngn.lib.player"
local texture = require "nngn.lib.texture"

local entities = {}
local warp
local tex <close> = texture.load("img/chrono_trigger/house.png")

local function init()
    -- warp
    table.insert(entities, entity.load(nil, nil, {
        pos = {-12, -84}, collider = {
            type = Collider.AABB, bb = 8,
            flags = Collider.TRIGGER}}))
    warp = entities[#entities]
    for _, t in ipairs({
        -- walls
        {{-24,  48}, {-56, -16, 56, 16}},
        {{-96, -32}, {-16, -64, 16, 64}},
        {{ 80, -16}, {-16, -32, 16, 32}},
        -- shelf
        {{ 48,  24}, {-16,  -8, 16,  8}},
        {{-40, -92}, { -8,  -4, 40,  4}},
        -- table
        {{-32,  24}, {-32,  -8, 32,  8}},
    }) do
        table.insert(entities, entity.load(nil, nil, {
            pos = t[1], collider = {
                type = Collider.AABB,
                bb = t[2], m = Math.INFINITY, flags = Collider.SOLID}}))
    end
    local w16, w32 = 512//16, 512//32
    for _, t in ipairs({
        -- walls
        {{-48, -88}, {-32, -40, 48, -8}, {96, 32}, {w32, w32}, {2, 4, 5, 5}},
        {{  8, -32}, { -8, -16,  8,  0}, {16, 32}, {w16, w32}, {5, 0}, 0},
        {{ 40, -48}, {-40,  -8, 24,  0}, {80, 32}, {w16, w32}, {5, 3, 10, 4}},
        -- stairs
        {{-24, -32}, {-24, -16, 24, -8}, {48, 32}, {w16, w32}, {0, 3, 3, 4}},
        -- bed
        {{ 40,  16}, {-16, -50, 16, -8}, {32, 16}, {w32, w16}, {1, 2}, -8},
        -- chair
        {{-32,  16}, { -8, -16,  8, -8}, {16, 32}, {w16, w32}, {4, 0}, nil},
    }) do
        table.insert(entities, entity.load(nil, nil, {
            pos = t[1],
            renderer = {
                type = Renderer.SPRITE,
                tex = tex.tex, size = t[3],
                scale = t[4], coords = t[5], z_off = t[6]},
            collider = {
                type = Collider.AABB,
                bb = t[2], m = Math.INFINITY, flags = Collider.SOLID}}))
    end
    -- stairs
    table.insert(entities, entity.load(nil, nil, {
        pos = {-29, -61},
        collider = {
            type = Collider.BB,
            bb = {-4, -22, 4, 22}, rot = math.rad(45), m = Math.INFINITY,
            flags = Collider.SOLID}}))
    -- light
    table.insert(entities, entity.load(nil, nil, {
        pos = {8, 64, 64}, light = {
            type = Light.POINT,
            color = {1, 1, 1, 1}, att = 2048, spec = 0}}))
    nngn:lighting():set_zsprites(true)
    nngn:renderers():set_zsprites(true)
    nngn:lighting():set_ambient_anim{
        rate_ms = 0, f = {
            type = AnimationFunction.LINEAR,
            v = 1, step_s = -5, ["end"] = .2}}
    player.move_all(-64, -128, false)
end

map.map {
    name = "house1",
    file = "maps/house1.lua",
    state = {zsprites = true},
    init = init,
    entities = entities,
    on_collision = function(e0, e1)
        if e0 == warp or e1 == warp then map.next("maps/house0.lua") end
    end,
    reset = function() nngn:lighting():set_ambient_anim({f = {}}) end,
    data = map.data(),
    tiles = {tex.tex, 2, 0, 0, 256, 256, 1, 1, {1, 1}},
}
