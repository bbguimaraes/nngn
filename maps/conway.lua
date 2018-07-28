local camera <const> = require "nngn.lib.camera"
local entity <const> = require "nngn.lib.entity"
local map <const> = require "nngn.lib.map"
local player <const> = require "nngn.lib.player"
local state <const> = require "nngn.lib.state"

local entities <const>, stars <const> = {}, {}
local warp

local conway <const> = require("maps/conway/conway"):new()
local maze <const> = require("maps/conway/maze"):new()
local chess <const> = require("maps/conway/chess"):new()

local function init()
    -- warp
    do
        local ts <const> = dofile("src/lson/pillar.lua")
        ts[1].pos = {0, 8}
        ts[2].pos = {0, 8}
        ts[1].collider = {
            type = Collider.AABB, bb = {-2, -10, 2, -6},
            flags = Collider.TRIGGER}
        table.insert(entities, entity.load(nil, nil, ts[1]))
        warp = entities[#entities]
        table.insert(entities, entity.load(nil, nil, ts[2]))
    end
    local star_renderer <const> = dofile("src/lson/star.lua")
    local coll_st <const> = Collider.SOLID | Collider.TRIGGER
    -- stars
    for _, t in ipairs{
        {{ 32,  32}, function() conway:restart() end},
        {{-32,  32}, function() maze:restart() end},
        {{-32, -32}, function() player.stop() chess:restart() end},
    } do
        local pos <const>, f <const> = table.unpack(t)
        local e = entity.load(nil, nil, {
            pos = pos,
            renderer = star_renderer.renderer,
            collider = {
                type = Collider.AABB,
                bb = 16, m = Math.INFINITY, flags = coll_st,
            },
        })
        table.insert(entities, e)
        stars[deref(e)] = f
    end
    local cell_size <const> = 8
    local cell_renderer <const> = {
        renderer = {
            type = Renderer.SPRITE, tex = 1,
            size = {cell_size - 2, cell_size - 2},
            scale = {512, 512}, z_off = cell_size,
        },
    }
    conway:init(cell_size, cell_renderer, {64, 64})
    maze:init(cell_size, cell_renderer, star_renderer, {-584, 64})
    chess:init(
        cell_size, cell_renderer,
        -9.5 * 4 * cell_size, -9.5 * 4 * cell_size)
    player.move_all(0, 32, true)
    local c <const> = camera.get()
    c:set_zoom(.5)
    local _, _, z <const> = c:pos()
    c:set_pos(0, 128, z)
    camera.set_follow(nil)
end

local function reset()
    camera.reset()
    conway:reset()
    maze:reset()
    chess:reset()
end

local function heartbeat()
    local dt_ms <const> = nngn:timing():dt_ms()
    conway:update(dt_ms)
    maze:update(dt_ms)
    chess:update(dt_ms)
end

local function on_collision(e0, e1)
    if e0 == warp or e1 == warp then
        map.next("maps/main.lua")
    end
    local f <const> = stars[deref(e0)] or stars[deref(e1)]
    if f then
        f()
    end
end

map.map {
    name = "conway",
    file = "maps/conway.lua",
    state = {camera_follow = true},
    init = init,
    entities = entities,
    reset = reset,
    heartbeat = heartbeat,
    on_collision = on_collision,
    tiles = {0, 1, 0, 0, 1, 1, 0, 0, {}},
}
