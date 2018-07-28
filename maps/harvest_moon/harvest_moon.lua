local entity = require "nngn.lib.entity"
local input = require "nngn.lib.input"
local map = require "nngn.lib.map"
local player = require "nngn.lib.player"
local textbox = require "nngn.lib.textbox"
local texture = require "nngn.lib.texture"

local grid <const> = require("maps.harvest_moon.grid"):new()

local entities, chickens, ripe_crops, score <const> = {}, {}, {}, {corn = 0}
local objects, tools, warp

local function chicken(grid, grid_pos, angry)
    local pos <const> = grid.abs_pos(grid_pos)
    local t <const> = dofile("src/lson/chicken.lua")
    t.pos = pos
    if angry then
        t.collider.flags = Collider.TRIGGER
    end
    local e <const> = entity.load(nil, nil, t)
    table.insert(entities, e)
    if angry then
        table.insert(chickens, e)
    end
end

local function init()
    local tex <close> = texture.load("img/harvest_moon.png")
    objects = require("maps.harvest_moon.objects"):new(grid, ripe_crops)
    tools = require("maps.harvest_moon.tools"):new(objects, chicken)
    -- warp
    table.insert(entities, entity.load(nil, nil, {
        pos = {-112,  16},
        collider = {
            type = Collider.AABB, bb = {-16, -32, 16, 32},
            flags = Collider.TRIGGER,
        },
    }))
    warp = entities[#entities]
    -- AABB solids
    for _, t in ipairs{
        -- north fence
        {{ 72, 248}, {-136, -8, 136, 8}},
        {{312, 232}, {-120, -8, 120, 8}},
        -- west fence
        {{-56, 152}, {-8, -88, 8, 88}},
        -- west house fence
        {{-8, 192}, {-8, -48, 8, 48}},
        -- house
        {{56, 168}, {-56, -40, 56, 40}},
        -- porch
        {{  6, 112}, { -6, -16,  6, 16}},
        {{104, 112}, { -8, -16,  8, 16}},
        {{ 17, 104}, { -5,  -8,  5,  8}},
        {{ 77, 104}, {-19,  -8, 19,  8}},
        -- materials
        {{128, 216}, {-16, -24, 16, 24}},
        -- north wall
        {{296, 216}, {-48, -8, 48, 8}},
        {{224, 200}, {-32, -8, 32, 8}},
        {{368, 200}, {-32, -8, 32, 8}},
        -- barn
        {{232, 152}, {-40, -40, 40, 40}},
        -- silo
        {{304, 160}, {-32, -48, 32, 48}},
        -- coop
        {{368, 152}, {-32, -40, 32, 40}},
        -- east fence
        {{424, 96}, {-8, -128, 8, 128}},
        -- well
        {{180, 30}, {-3, -3, 3, 3}},
        {{220, 30}, {-3, -3, 3, 3}},
        -- north entrance fence
        {{-40, 56}, {-56, -8, 56, 8}},
        {{ 56, 56}, { -8, -8,  8, 8}},
        -- east entrance fence
        {{68, 16}, {-4, -48, 4, 48}},
        -- produce box
        {{48, -16}, {-16, -16, 16, 16}},
        -- south fence
        {{-72, -24}, { -24, -8,  24, 8}},
        {{408, -24}, {  -8, -8,   8, 8}},
        {{-60, -40}, {  -4, -8,   4, 8}},
        {{204, -40}, {-212, -8, 212, 8}},
        {{-32, -56}, { -32, -8,  32, 8}},
    } do
        local pos <const>, bb <const> = table.unpack(t)
        table.insert(entities, entity.load(nil, nil, {
            pos = pos,
            collider = {
                type = Collider.AABB,
                bb = bb, m = Math.INFINITY, flags = Collider.SOLID,
            },
        }))
    end
    -- sphere solids
    for _, t in ipairs{
        {{200,  36}, 16},
        {{304, 130}, 32},
    } do
        local pos <const>, r <const> = table.unpack(t)
        table.insert(entities, entity.load(nil, nil, {
            pos = pos,
            collider = {
                type = Collider.SPHERE,
                r = r, m = Math.INFINITY, flags = Collider.SOLID,
            },
        }))
    end
    -- objects
    for _, t in ipairs{
        -- signs
        {{ -8, -24}, {16, 16}, {8, 0}, 0, {-8, -8, 8, 8}},
        {{ 24,  -8}, {16, 16}, {8, 0}, 0, {-8, -8, 8, 8}},
        {{152, 136}, {16, 16}, {8, 0}, 0, {-8, -8, 8, 8}},
        {{152, 216}, {16, 16}, {8, 0}, 0, {-8, -8, 8, 8}},
        {{280, 104}, {16, 16}, {8, 0}, 0, {-8, -8, 8, 8}},
        -- well
        {{200, 40}, {48, 48}, {14, 3, 17, 6}, -14},
        -- tools
        {{328, 24}, {80, 80}, {23, 1, 28, 6}, -40, {-40, -40, 40, 8}},
        -- stable
        {{176, 160}, {32, 64}, {15, 10, 17, 14}, -32, {-16, -32, 16, 8}},
    } do
        local
            pos <const>, size <const>, coords <const>, z_off <const>,
            bb <const> =
                table.unpack(t)
        local collider
        if bb then
            collider = {
                type = Collider.AABB, flags = Collider.SOLID,
                bb = bb, m = Math.INFINITY,
            }
        end
        table.insert(entities, entity.load(nil, nil, {
            pos = pos,
            renderer = {
                type = Renderer.SPRITE, tex = tex.tex,
                size = size, coords = coords, scale = {512//16, 512//16},
                z_off = z_off,
            },
            collider = collider,
        }))
    end
    objects:create(objects.TYPE.ROCK, 5, -1)
    -- animals
    for _, t in ipairs{
        {"src/lson/cow.lua",     {256, 104}},
        {"src/lson/horse.lua",   {176, 128}},
        {"src/lson/dog.lua",     { 22, 128}},
        {"src/lson/chicken.lua", {376, 112}},
        {"src/lson/bird.lua",    { 90, 128}},
    } do
        local f <const>, pos <const> = table.unpack(t)
        local t <const> = dofile(f)
        t.pos = pos
        t.collider.flags = Collider.SOLID
        t.collider.m = Math.INFINITY
        table.insert(entities, entity.load(nil, nil, t))
    end
    player.move_all(-36, 32, false)
    nngn:camera():set_limits(
        -96, -96, -Math.INFINITY,
        480, 320,  Math.INFINITY)
end

local function reset()
    tools:reset()
    objects:reset()
end

local function heartbeat()
    local dt <const> = nngn:timing():fdt_ms()
    tools:update(dt)
    objects:update(dt)
end

local function key_callback(key, press, mods)
    if tools:active() then
        return
    end
    if key == string.byte("E") then
        if press then
            local e <const> = player.entity()
            if e then
                tools:activate(e)
            end
        end
        return true
    elseif key == string.byte("F") then
        if press then
            local p <const> = player.cur()
            if p then
                tools:action(grid:get_target(p))
            end
        end
        return true
    end
end

local function on_collision(e0, e1)
    if e0 == warp or e1 == warp then
        return map.next("maps/main.lua")
    end
    local ripe <const> = ripe_crops[deref(e0)] or ripe_crops[deref(e1)]
    if ripe then
        score.corn = score.corn + 1
        textbox.update("corn", tostring(score.corn))
        objects:transform_obj(objects.TYPE.SOIL, ripe)
        ripe_crops[deref(e0)] = nil
        ripe_crops[deref(e1)] = nil
    end
end

map.map {
    name = "hm",
    file = "maps/harvest_moon.lua",
    state = {camera = true},
    init = init,
    reset = reset,
    entities = entities,
    heartbeat = heartbeat,
    key_callback = key_callback,
    on_collision = on_collision,
    tiles = {
        "img/harvest_moon.png",
        32, grid.offset[1], grid.offset[2],
        16, 16, grid.size[1], grid.size[2],
        dofile("maps/harvest_moon/map.lua"),
    },
}
