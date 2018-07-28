local entity = require "nngn.lib.entity"
local map = require "nngn.lib.map"
local player = require "nngn.lib.player"
local textbox = require "nngn.lib.textbox"
local texture = require "nngn.lib.texture"

local entities = {}
local old_man, warps

local function init()
    local tex <close> = texture.load("img/chrono_trigger/end_of_time.png")
    -- old man
    do
        local t = dofile("src/lson/old_man.lua")
        t.pos = {-24, 0}
        t.collider.flags = Collider.TRIGGER | Collider.SOLID
        t.collider.m = Math.INFINITY
        table.insert(entities, entity.load(nil, nil, t))
    end
    old_man = entities[#entities]
    -- warps
    warps = {}
    for i = 0, 8 do
        if i ~= 4 then
            local x, y = i % 3, i // 3
            table.insert(entities, entity.load(nil, nil, {
                pos = {x * -32 + y * -16 - 24, x * 16 + y * -32 + 12},
                collider = {
                    type = Collider.AABB, bb = 4,
                    flags = Collider.TRIGGER}}))
            table.insert(warps, entities[#entities])
        end
    end
    table.insert(entities, entity.load(nil, nil, {
        pos = {264, -132},
        collider = {
            type = Collider.AABB,
            bb = 16, m = Math.INFINITY, flags = Collider.SOLID}})) --TRIGGER}}))
--    table.insert(warps, entities[#entities])
    table.insert(entities, entity.load(nil, nil, {
        pos = {219, -82},
        collider = {
            type = Collider.BB,
            bb = {-16, -4, 16, 4}, rot = math.rad(153.5), m = Math.INFINITY,
            flags = Collider.SOLID}})) --TRIGGER}}))
--    table.insert(warps, entities[#entities])
    -- pillars
    do
        local ts = dofile("src/lson/pillar.lua")
        ts[1].tex = tex.tex
        ts[2].tex = tex.tex
        local anims = nngn.animations:load_v({ts[1].anim, ts[2].anim})
        ts[1].anim.sprite = anims[1]:sprite()
        ts[2].anim.sprite = anims[2]:sprite()
        local pos = {
            {-24,  20}, {-56,  36}, { -88,  52},
            {-40, -12}, {-72,   4}, {-104,  20},
            {-56, -44}, {-88, -28}, {-120, -12},
        }
        for _, pos in ipairs(pos) do
            ts[1].pos = pos
            ts[2].pos = pos
            table.insert(entities, entity.load(nil, nil, ts[1]))
            table.insert(entities, entity.load(nil, nil, ts[2]))
        end
        nngn.animations:remove_v(anims)
    end
    -- fences
    for _, pos in ipairs({
        {-168,  -22}, {-136,  -38}, {-104,  -54}, {-72,  -70},
        {  88, -182}, { 120, -198}, { 152, -214}, {184, -230},
    }) do
        table.insert(entities, entity.load(nil, nil, {
            pos = pos, renderer = {
                type = Renderer.SPRITE,
                tex = tex.tex, size = {32, 64}, z_off = -36,
                scale = {512//32, 512//64}, coords = {2, 1}}}))
    end
    -- corner fences
    for _, pos in ipairs({{-40, -86}, {216, -246}}) do
        table.insert(entities, entity.load(nil, nil, {
            pos = pos, renderer = {
                type = Renderer.SPRITE,
                tex = tex.tex, size = {32, 64}, z_off = -36,
                scale = {512//32, 512//64}, coords = {3, 1}}}))
    end
    -- vertical fences
    for _, pos in ipairs({{-16, -70}, {240, -230}}) do
        table.insert(entities, entity.load(nil, nil, {
            pos = pos, renderer = {
                type = Renderer.SPRITE,
                tex = tex.tex, size = {32, 96},
                scale = {512//32, 512//32}, coords = {4, 2, 5, 5}}}))
    end
    table.insert(entities, entity.load(nil, nil, {
        pos = {32, 26}, renderer = {
            type = Renderer.SPRITE,
            tex = tex.tex, size = {32, 96},
            scale = {512//32, 512//32}, coords = {5, 2, 6, 5}}}))
    -- stairs/door
    table.insert(entities, entity.load(nil, nil, {
        pos = {48, -61}, renderer = {
            type = Renderer.SPRITE,
            tex = tex.tex, size = {128, 128},
            scale = {512//128, 512//128}, coords = {0, 1}}}))
    -- bridge
    table.insert(entities, entity.load(nil, nil, {
        pos = {296, -220}, renderer = {
            type = Renderer.SPRITE,
            tex = tex.tex, size = {96, 96},
            scale = {512//32, 512//32}, coords = {0, 8, 3, 11}}}))
    table.insert(entities, entity.load(nil, nil, {
        pos = {312, -204}, renderer = {
            type = Renderer.SPRITE,
            tex = tex.tex, size = {128, 128}, z_off = 64,
            scale = {512//128, 512//128}, coords = {3, 0}}}))
    -- lamp post
    table.insert(entities, entity.load(nil, nil, {
        pos = {184, -124},
        renderer = {type = Renderer.SPRITE, tex = tex.tex, size = {16, 96}},
        anim = {
            sprite = {512//16, 512//32, {{
                {1, 0, 2, 3, 300}, {2, 0, 3, 3, 100},
                {3, 0, 4, 3, 200}, {2, 0, 3, 3, 200},
                {3, 0, 4, 3, 200}, {2, 0, 3, 3, 200},
                {1, 0, 2, 3, 300}, {2, 0, 3, 3, 100},
                {3, 0, 4, 3, 200}, {2, 0, 3, 3, 200},
                {3, 0, 4, 3, 200}, {2, 0, 3, 3, 200},
                {1, 0, 2, 3, 300}, {2, 0, 3, 3, 100},
                {3, 0, 4, 3, 200}, {2, 0, 3, 3, 200},
                {3, 0, 4, 3, 200}, {2, 0, 3, 3, 200},
                {3, 0, 4, 3, 200}, {2, 0, 3, 3, 200}}}}}}))
    table.insert(entities, entity.load(nil, nil, {
        pos = {184, -164},
        collider = {
            type = Collider.SPHERE,
            r = 8, m = Math.INFINITY, flags = Collider.SOLID}}))
    -- floor
    table.insert(entities, entity.load(nil, nil, {
        pos = {184, -164},
        renderer = {
            type = Renderer.SPRITE,
            tex = tex.tex, size = {64, 64}, z_off = 32},
        anim = {
            sprite = {512//64, 512//64, {{
                {1, 0, 300}, {2, 0, 500}, {1, 0, 200}, {2, 0, 200},
                {1, 0, 300}, {2, 0, 500}, {1, 0, 200}, {2, 0, 200},
                {1, 0, 300}, {2, 0, 500}, {1, 0, 200}, {2, 0, 200},
                {1, 0, 200}, {2, 0, 200}}}}}}))
    -- solids
    for _, v in ipairs({
        {{  32,    7}, { -4, -24,  4, 24}, 333.5},
        {{ -36,   69}, {-84,  -4, 84,  4}, 153.5},
        {{-152,   33}, { -4, -80,  4, 80}, 333.5},
        {{-107,  -83}, {-88,  -4, 88,  4}, 153.5},
        {{ -12,  -83}, { -4, -38,  4, 38}, 333.5},
        {{  27,  -75}, {-30,  -4, 30,  4}, 315.0},
        {{  75, -109}, {-30,  -4, 30,  4}, 333.5},
        {{  88, -159}, { -4, -43,  4, 43}, 333.5},
        {{ 147, -241}, {-85,  -4, 85,  4}, 153.5},
        {{ 247, -240}, { -4, -42,  4, 42}, 333.5},
        {{ 292, -220}, {-28,  -4, 28,  4}, 153.5},
        {{ 326, -244}, { -4, -16,  4, 16},  40.0},
        {{ 350, -242}, { -4, -16,  4, 16}, 333.5},
        {{ 348, -213}, { -4, -16,  4, 16},  40.0},
        {{ 304, -184}, {-36,  -4, 36,  4}, 153.5},
        {{ 286, -148}, { -4, -20,  4, 20}, 333.5},
        {{ 174,  -68}, {-30,  -4, 30,  4}, 153.5},
        {{ 260, -112}, {-34,  -4, 34,  4}, 153.5},
        {{ 133,  -72}, { -4, -18,  4, 18}, 153.5},
        {{  95,  -68}, {-32,  -4, 32,  4}, 333.5},
        {{  45,  -32}, {-32,  -4, 32,  4}, 315.0},
    }) do
        table.insert(entities, entity.load(nil, nil, {
            pos = v[1], collider = {
                type = Collider.BB,
                bb = v[2], rot = math.rad(v[3]),
                m = Math.INFINITY, flags = Collider.SOLID}}))
    end
    for _, pos in ipairs({{ 104, -160}, { 280, -144}}) do
        table.insert(entities, entity.load(nil, nil, {
            pos = pos, collider = {
                type = Collider.AABB,
                bb = {-8, -12, 8, 12}, m = Math.INFINITY,
                flags = Collider.SOLID}}))
    end
    if map.name() then
        player.move_all(-72, -4, true)
    else
        player.move_all(0, 0, true)
    end
end

local function on_collision(e0, e1)
    if e0 == old_man or e1 == old_man then
        textbox.update("old man", [[
Ah, more guests...! Why, this is "The end of
Time", of course!]])
    elseif e0 == warps[1] or e1 == warps[1] then
        map.next("maps/house0.lua")
    elseif e0 == warps[2] or e1 == warps[2] then
        map.next("maps/fe0.lua")
    elseif e0 == warps[3] or e1 == warps[3] then
        map.next("maps/zelda.lua")
    elseif e0 == warps[4] or e1 == warps[4] then
        map.next("maps/hm.lua")
    elseif e0 == warps[5] or e1 == warps[5] then
        map.next("maps/wolfenstein.lua")
    elseif e0 == warps[6] or e1 == warps[6] then
        map.next("maps/light.lua")
    elseif e0 == warps[7] or e1 == warps[7] then
        map.next("maps/coll.lua")
    elseif e0 == warps[8] or e1 == warps[8] then
        map.next("maps/conway.lua")
    end
end

map.map {
    name = "main",
    file = "maps/main.lua",
    init = init,
    entities = entities,
    on_collision = on_collision,
    tiles = {
        "img/chrono_trigger/chrono_end.png",
        1, 72, -96, 512, 512, 1, 1, {0, 0},
    },
}
