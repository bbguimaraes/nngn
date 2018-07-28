local entity = require "nngn.lib.entity"
local map = require "nngn.lib.map"
local player = require "nngn.lib.player"

local entities = {}
local warp, racket_v, walls, rackets, ball

local function init()
    -- warp
    table.insert(entities, entity.load(nil, nil, {
        pos = {0, -64}, collider = {
            type = Collider.AABB, bb = 32, flags = Collider.TRIGGER}}))
    warp = entities[#entities]
    -- walls
    walls = {}
    for _, t in ipairs({
        {{   0,  144}, {256,  32}, {-128,  -16, 128,  16}},
        {{-144,    0}, { 32, 256}, { -16, -128,  16, 128}},
        {{ 144,    0}, { 32, 256}, { -16, -128,  16, 128}},
        {{   0, -144}, {256,  32}, {-128,  -16, 128,  16}},
    }) do
        local e = entity.load(nil, nil, {
        pos = t[1],
        collider = {
            type = Collider.AABB,
            bb = t[3], m = Math.INFINITY, flags = Collider.SOLID},
        renderer = {
            type = Renderer.SPRITE, tex = 1, size = t[2],
            scale = {512, 512}, coords = {1, 0}}})
        table.insert(entities, e)
        table.insert(walls, e)
    end
    -- rackets
    racket_v =
        function(s) return {0, s * 128 * (nngn.math:rand() / 2 + .5), 0} end
    for _, t in ipairs({
        {{-96, 0}, { 8, 64}, { -4, -32, 4, 32}},
        {{ 96, 0}, { 8, 64}, { -4, -32, 4, 32}},
    }) do
        table.insert(entities, entity.load(nil, nil, {
            pos = t[1], vel = racket_v(nngn.math:rand_int(0, 1) * 2 - 1),
            collider = {
                type = Collider.AABB,
                bb = t[3], m = Math.INFINITY, flags = Collider.SOLID},
            renderer = {
                type = Renderer.SPRITE, tex = 1, size = t[2], z_off = 32,
                scale = {512, 512}, coords = {1, 0}}}))
    end
    rackets = {entities[#entities - 1], entities[#entities]}
    -- ball
    do
        local v = function()
            local v, a = 256, nngn.math:rand() * 2 * math.pi
            return {v * math.cos(a), v * math.sin(a), 0}
        end
        table.insert(entities, entity.load(nil, "src/lson/circle8.lua", {
            vel = v(),
            collider = {
                type = Collider.SPHERE, r = 4,
                flags = Collider.SOLID | Collider.TRIGGER}}))
    end
    ball = entities[#entities]
    -- cube
    table.insert(entities, entity.load(nil, nil, {
        pos = {0, 64, 8},
        collider = {type = Collider.AABB, bb = 16, flags = Collider.SOLID},
        renderer = {type = Renderer.CUBE, size = 16, color = {1, .5, 0}}}))
    -- spheres
    for _, pos in ipairs({{-64, 64}, {64, 64}}) do
        table.insert(entities,
            entity.load(nil, "src/lson/circle32.lua", {
                pos = pos,
                collider = {
                    type = Collider.SPHERE, r = 16, flags = Collider.SOLID}}))
    end
    -- trees
    for _, v in ipairs({
        {"src/lson/rock0.lua", {-64, 16}},
        {"src/lson/tree0.lua", { 64, 32}},
    }) do
        local t = dofile(v[1])
        t.pos = v[2]
        t.collider.flags = Collider.SOLID
        table.insert(entities, entity.load(nil, nil, t))
    end
    -- solids
    for _, t in ipairs({
        {{-8, -8}, {8, 8}, {-4, -4, 4, 4}},
        {{ 8, -8}, {4, 8}, {-2, -4, 2, 4}},
        {{-8,  8}, {4, 4}, {-2, -2, 2, 2}},
    }) do
        table.insert(entities, entity.load(nil, nil, {
            pos = t[1],
            collider = {
                type = Collider.AABB, bb = t[3],
                flags = Collider.SOLID},
            renderer = {
                type = Renderer.SPRITE, tex = 1, size = t[2],
                scale = {512, 512}, coords = {0, 0}}}))
    end
    player.move_all(0, 0, true)
end

local function heartbeat()
    for _, r in ipairs(rackets) do
        local _, y = r:pos()
        if y < -96 then r:set_vel(table.unpack(racket_v(1)))
        elseif y > 96 then r:set_vel(table.unpack(racket_v(-1))) end
    end
end

local function on_collision(e0, e1, v)
    if e0 == ball or e1 == ball then
        local o = e0 == ball and e1 or e0
        if o ~= rackets[1] and o ~= rackets[2]
                and o ~= walls[1] and o ~= walls[2]
                and o ~= walls[3] and o ~= walls[4]
        then return end
        local vel = {ball:vel()}
        if v[1] ~= 0 or v[2] ~= 0 then
            if math.abs(v[1]) > math.abs(v[2])
            then vel[1] = -vel[1]
            else vel[2] = -vel[2] end
        end
        ball:set_vel(table.unpack(vel))
    elseif e0 == warp or e1 == warp then
        map.next("maps/main.lua")
    end
end

map.map {
    name = "coll",
    file = "maps/coll.lua",
    init = init,
    entities = entities,
    heartbeat = heartbeat,
    on_collision = on_collision,
    tiles = {0, 1, 0, 0, 1, 1, 0, 0, {}},
}
