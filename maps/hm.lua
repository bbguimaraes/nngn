local animation = require "nngn.lib.animation"
local entity = require "nngn.lib.entity"
local input = require "nngn.lib.input"
local map = require "nngn.lib.map"
local player = require "nngn.lib.player"
local texture = require "nngn.lib.texture"

local vec2 = require("nngn.lib.math").vec2

local grid_size, scale = 16, {512//16, 512//16}
local entities, chickens, chicken_flock = {}, {}, {}
local tools, flock, warp

local OBJ_TYPE <const> = {
    EMPTY = 0,
    WET_SOIL = 1,
    WEED = 2,
    SOIL = 3,
    ROCK = 4,
    STUMP = 5,
}
local TOOL_TYPE <const> = {
    WATERING_CAN = 0,
    SICKLE = 1,
    HOE = 2,
    HAMMER = 3,
    AXE = 4,
    SPRINKLER = 5,
    GOLDEN_SICKLE = 6,
    GOLDEN_HOE = 7,
    GOLDEN_HAMMER = 8,
    GOLDEN_AXE = 9,
}

local function chicken(pos, angry)
    local t = dofile("src/lson/chicken.lua")
    t.pos = pos
    if angry then
        t.collider.flags = Collider.TRIGGER
    end
    local e = entity.load(nil, nil, t)
    table.insert(entities, e)
    if angry then
        table.insert(chickens, e)
    end
end

local grid = {
    size = {36, 26},
    offset = {192, 112},
}

function grid:get_target()
    local p <const> = nngn.players:cur()
    local e <const> = p:entity()
    local pos <const> = vec2(e:pos())
        + vec2(p:face_vec(1.125 * grid_size))
        + vec2(0, 0.5 * e:renderer():z_off())
    local grid_pos =
        (pos / vec2(grid_size)):map(self.round)
    grid_pos[3] = 0
    return grid_pos
end

function grid.round(x)
    if x < 0 then return math.ceil(x - 1)
    else return math.floor(x) end
end

function grid.abs_pos(grid_pos)
    local ret = vec2(grid_size) * (grid_pos + vec2(0.5))
    ret[3] = 0
    return ret
end

local objects = {
    v = {},
}

function objects:reset()
    for _, x in pairs(self.v) do
        nngn:remove_entity(x.entity)
    end
end

function objects:idx(pos)
    return pos[2] * grid.size[1] + pos[1]
end

function objects.load_entity(type, e)
    local t = {
        renderer = {
            type = Renderer.SPRITE, tex = "img/harvest_moon.png",
            scale = scale, z_off = 8,
        },
    }
    if type == OBJ_TYPE.WET_SOIL then
        t.renderer.coords = {7, 1}
        t.renderer.size = {16, 16}
    elseif type == OBJ_TYPE.WEED then
        t.renderer.coords = {0, 2}
        t.renderer.size = {16, 16}
    elseif type == OBJ_TYPE.SOIL then
        t.renderer.coords = {7, 0}
        t.renderer.size = {16, 16}
    elseif type == OBJ_TYPE.ROCK then
        t.renderer.coords = {0, 3}
        t.renderer.size = {16, 16}
        t.collider = {
            type = Collider.AABB, flags = Collider.SOLID,
            bb = {-8, -8, 8, 8}, m = Math.INFINITY,
        }
    else
        return
    end
    return entity.load(e, nil, t)
end

function objects:create(type, pos)
    local i = self:idx(pos)
    if self.v[i] then return end
    local e = objects.load_entity(type)
    if not e then return end
    e:set_pos(table.unpack(grid.abs_pos(pos)))
    local t = {type = type, entity = e}
    self.v[i] = t
end

function objects:destroy(type, pos)
    local i = self:idx(pos)
    local obj = self.v[i]
    if not obj or type ~= obj.type then return end
    local e = obj.entity
    nngn:remove_entity(e)
    self.v[i] = nil
end

function objects:transform(from, to, pos)
    local obj = self.v[self:idx(pos)]
    if not obj or from ~= obj.type then return end
    local e = obj.entity
    nngn.renderers:remove(e:renderer())
    e:set_renderer(nil)
    local c = e:collider()
    if c then
        nngn.colliders:remove(c)
        e:set_collider(nil)
    end
    objects.load_entity(to, e)
end

local tools = {
    n = 10,
    active = false,
    positions = {},
    actions = {},
    entities = {},
    pos = 0,
}

function tools:init()
    self:init_positions()
    self:init_actions()
    self:init_input()
end

function tools:init_positions()
    local radius = 32
    local start = math.pi / 2
    for i = 0, self.n - 1 do
        local angle = start + 2 * math.pi * (i / self.n)
        table.insert(self.positions, {
            radius * math.cos(angle),
            radius * math.sin(angle),
        })
    end
end

function tools:init_actions()
    local t = TOOL_TYPE
    self.actions = {
        [t.WATERING_CAN + 1] = function(pos)
            objects:transform(OBJ_TYPE.SOIL, OBJ_TYPE.WET_SOIL, pos)
        end,
        [t.SICKLE + 1] = function(pos)
            objects:destroy(OBJ_TYPE.WEED, pos)
        end,
        [t.HOE + 1] = function(pos)
            objects:create(OBJ_TYPE.SOIL, pos)
        end,
        [t.HAMMER + 1] = function(pos)
            objects:destroy(OBJ_TYPE.ROCK, pos)
        end,
        [t.AXE + 1] = function(pos)
            objects:destroy(OBJ_TYPE.STUMP, pos)
        end,
        [t.SPRINKLER + 1] = function(pos)
            chicken(grid.abs_pos(pos), false)
        end,
        [t.GOLDEN_SICKLE + 1] = function(pos)
            objects:create(OBJ_TYPE.WEED, pos)
        end,
        [t.GOLDEN_HOE + 1] = function(pos)
            chicken(grid.abs_pos(pos), true)
        end,
        [t.GOLDEN_HAMMER + 1] = function(pos)
            objects:create(OBJ_TYPE.ROCK, pos)
        end,
        [t.GOLDEN_AXE + 1] = function(pos)
            objects:create(OBJ_TYPE.STUMP, pos)
        end,
    }
end

function tools:init_input()
    local i = BindingGroup:new()
    i:add(string.byte("E"), Input.SEL_PRESS, function(_, _, mods)
        if mods & Input.MOD_SHIFT == 0 then
            self:cycle(1)
        else
            self:cycle(-1)
        end
    end)
    i:add(string.byte("F"), Input.SEL_PRESS, function() self:dismiss() end)
    self.input = i
end

function tools:reset()
    for _, x in ipairs(self.entities) do
        nngn:remove_entity(x)
    end
    self.entities = {}
end

function tools:heartbeat()
    if self.active then self:update_pos() end
end

function tools:load(e)
    local tex <close> = texture.load("img/harvest_moon.png")
    for y = 0, 1 do
        for x = 0, 4 do
            table.insert(self.entities, entity.load(nil, nil, {
                parent = e,
                pos = self.positions[5 * y + x + 1],
                renderer = {
                    type = Renderer.SPRITE, tex = tex.tex,
                    size = {16, 16}, scale = scale, z_off = -8,
                    coords = {14 + x, 30 + y}
                },
            }))
        end
    end
end

function tools:update_pos()
    local n = #self.entities
    for i, e in ipairs(self.entities) do
        local x, y = table.unpack(
            self.positions[(i + n - self.pos - 1) % n + 1])
        e:set_pos(x, y, 0)
    end
end

function tools:activate()
    if self.active then return end
    local p = nngn.players:cur()
    if not p then return end
    local e = p:entity()
    self:load(e)
    self:update_pos()
    self.active = true
    nngn.input:set_binding_group(self.input)
end

function tools:dismiss()
    self.active = false
    self:reset()
    nngn.input:set_binding_group(input.input)
end

function tools:cycle(dir)
    if not self.active then return end
    local n = #self.entities
    self.pos = (self.pos + n + dir) % n
    self:update_pos()
end

function tools:action()
    if self.active then return end
    local grid_pos = grid:get_target()
    self.actions[self.pos + 1](grid_pos)
end

local function init()
    local tex <close> = texture.load("img/harvest_moon.png")
    tools:init()
    -- warp
    table.insert(entities, entity.load(nil, nil, {
        pos = {-112,  16}, collider = {
            type = Collider.AABB, bb = {-16, -32, 16, 32},
            flags = Collider.TRIGGER}}))
    warp = entities[#entities]
    -- solids
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
        -- stable
        {{176, 160}, {-16, -32, 16, 32}},
        -- barn
        {{232, 152}, {-40, -40, 40, 40}},
        -- silo
        {{304, 152}, {-32, -56, 32, 56}},
        -- coop
        {{368, 152}, {-32, -40, 32, 40}},
        -- east fence
        {{424, 96}, {-8, -128, 8, 128}},
        -- well
        {{200, 40}, {-24, -24, 24, 24}},
        -- tools
        {{328, 24}, {-40, -40, 40, 40}},
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
        local pos, bb = table.unpack(t)
        table.insert(entities, entity.load(nil, nil, {
            pos = pos, collider = {
                type = Collider.AABB,
                bb = bb, m = Math.INFINITY, flags = Collider.SOLID}}))
    end
    -- objects
    for _, t in ipairs{
        -- signs
        {{-8, -24}, {8, 0}},
        {{24,  -8}, {8, 0}},
    } do
        local pos, coords = table.unpack(t)
        table.insert(entities, entity.load(nil, nil, {
            pos = pos,
            renderer = {
                type = Renderer.SPRITE, tex = tex.tex,
                size = {16, 16}, coords = coords, scale = scale},
            collider = {
                type = Collider.AABB, flags = Collider.SOLID,
                bb = {-8, -8, 8, 8}, m = Math.INFINITY}}))
    end
    objects:create(OBJ_TYPE.ROCK, {5, -1})
    -- animals
    for _, t in ipairs{
        {"src/lson/cow.lua",     {-64, 34}},
        {"src/lson/horse.lua",   {-32, 36}},
        {"src/lson/dog.lua",     { -8, 34}},
        {"src/lson/chicken.lua", { 17, 32}},
        {"src/lson/bird.lua",    { 40, 32}},
    } do
        local f, pos = table.unpack(t)
        local t = dofile(f)
        t.pos = pos
        t.collider.flags = Collider.SOLID
        t.collider.m = Math.INFINITY
        table.insert(entities, entity.load(nil, nil, t))
    end
    player.move_all(-36, 32, false)
end

local function reset()
    tools:reset()
    objects:reset()
end

local function heartbeat()
    tools:heartbeat()
    if flock then
        flock = flock:update(nngn.timing:dt_ms())
    end
end

local function key_callback(key, press, mods)
    if tools.active then return end
    if key == string.byte("E") then
        if press then tools:activate() end
        return true
    elseif key == string.byte("F") then
        if press then tools:action() end
        return true
    end
end

local function on_collision(e0, e1)
    if e0 == warp or e1 == warp then
        return map.next("maps/main.lua")
    end
    for i, x in ipairs(chickens) do
        if e0 ~= x and e1 ~= x then goto continue end
        table.remove(chickens, i)
        table.insert(chicken_flock, x)
        nngn.renderers:remove(x:renderer())
        nngn.colliders:remove(x:collider())
        x:set_renderer(nil)
        x:set_collider(nil)
        local t = dofile("src/lson/chicken.lua")
        local a = t.anim.sprite[3]
        a[1][1] = {0, 28, 100}
        a[1][2] = {1, 28, 100}
        entity.load(x, nil, t)
        flock = animation.flock(
            nngn.players:cur():entity(),
            chicken_flock, 4 * player.MAX_VEL, 0)
        break
        ::continue::
    end
end

map.map {
    name = "hm",
    init = init,
    reset = reset,
    entities = entities,
    heartbeat = heartbeat,
    key_callback = key_callback,
    on_collision = on_collision,
    tiles = {
        "img/harvest_moon.png",
        32, grid.offset[1], grid.offset[2],
        16, 16, grid.size[1], grid.size[2], {
            0, 0,   5, 3,   6, 5,   6, 5,   6, 5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   0,  5,   0,  5,   6,  5,  6,  5,  0,  5,  0,  5,
            0, 0,   5, 3,   6, 5,   6, 5,   6, 5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   5,  5,  28,  0,  29,  0, 30,  0, 31,  0,  7,  5,
            0, 0,   5, 3,   0, 5,   0, 5,   0, 5,   0,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   5,  5,  28,  1,  29,  1, 30,  1, 31,  1,  7,  5,
            0, 1,   5, 4,   1, 0,   2, 0,   3, 0,   4,  0,   3,  5,   3,  5,   3,  5,   3,  5,   0,  5,   0,  4,   0,  4,   0,  4,   0,  4,   0,  4,   0,  4,   0,  4,   0,  4,   0,  4,   0,  4,   0,  4,   0,  4,   0,  4,   0,  4,   0,  4,   0,  4,   0,  4,   0,  4,   0,  4,   0,  5,   0,  5,  29,  2, 30,  2, 31,  2,  0,  5,
            0, 5,   3, 5,   0, 5,   2, 3,   2, 3,   2,  3,   2,  1,   2,  1,   5,  0,   6,  0,   5,  5,   6,  3,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   1,  1,   7,  7,   7,  7,   2,  1,   2,  1,  19,  8,   3,  1,   6,  5,   6,  5,   6,  5,   0,  4,   0,  4,  6,  5,  6,  5,  6,  5,
            5, 3,   1, 1,   2, 1,   2, 3,   2, 3,   2,  3,   2,  3,   2,  3,   5,  1,   6,  1,   5,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,  20,  6,   6,  5,  22,  1,  23,  1,  24,  1,  25,  1,  26,  1,  27,  1,   6,  5,   6,  5,   6,  5,   0,  4,  6,  5,  6,  5,  6,  5,
            5, 3,   1, 2,   2, 3,   2, 3,   2, 3,   2,  3,   2,  3,   2,  3,   2,  3,   3,  2,   5,  5,   6,  3,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,  20,  6,   6,  5,  22,  2,  23,  2,  24,  2,  25,  2,  26,  2,  27,  2,   6,  5,   6,  5,   6,  5,   0,  4,  6,  5,  6,  5,  6,  5,
            5, 3,   1, 3,   2, 3,   2, 3,   2, 3,   2,  3,   2,  3,   2,  3,   2,  3,   3,  3,   5,  5,   6,  4,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,  14,  3,  15,  3,  16,  3,   6,  5,  20,  6,   6,  5,  22,  3,  23,  3,  24,  3,  25,  3,  26,  3,  27,  3,   6,  5,   6,  5,   6,  5,   0,  4,  6,  5,  6,  5,  6,  5,
            5, 3,   1, 4,   2, 4,   2, 4,   2, 4,   2,  4,   2,  4,   2,  4,   2,  4,   3,  4,   5,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,  14,  4,  15,  4,  16,  4,   6,  5,  20,  6,   6,  5,  22,  4,  23,  4,  24,  4,  25,  4,  26,  4,  27,  4,   6,  5,   6,  5,   6,  5,   0,  4,  6,  5,  6,  5,  6,  5,
            0, 5,   1, 5,   2, 5,   2, 5,   2, 5,   2,  5,   4,  5,   5,  6,   6,  6,   4,  5,   5,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,  14,  5,  15,  5,  16,  5,   6,  5,  20,  6,   6,  5,   6,  5,  23,  5,  24,  5,  25,  5,  26,  5,  27,  5,   6,  5,   6,  5,   6,  5,   0,  4,  6,  5,  6,  5,  6,  5,
            0, 6,   5, 3,   0, 4,   6, 5,   6, 5,   6,  5,   6,  5,   5,  6,   6,  6,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,  20,  6,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   0,  4,  6,  5,  6,  5,  6,  5,
            0, 7,   5, 3,   0, 4,   6, 5,   6, 5,   6,  5,   6,  5,   5,  7,   6,  7,   7,  7,   7,  7,   7,  7,   7,  7,   7,  7,   7,  7,   7,  7,   7,  7,   7,  7,   2,  1,   2,  1,   2,  1,  20,  7,  21,  7,   7,  7,   7,  7,   7,  7,   7,  7,   2,  1,   2,  1,   3,  1,   6,  5,   6,  5,   0,  4,  6,  5,  6,  5,  6,  5,
            0, 8,   5, 3,   0, 4,   6, 5,   6, 5,   6,  5,   5,  8,   6,  8,   7,  8,   8,  8,   9,  8,  10,  8,  11,  8,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   1,  2,   2,  3,  19,  8,   2,  3,   3,  2,  22,  8,  23,  8,  24,  8,  25,  8,   2,  3,  19,  8,   3,  2,   6,  5,   6,  5,   0,  4,  6,  5,  6,  5,  6,  5,
            0, 8,   5, 3,   0, 4,   6, 5,   6, 5,   6,  5,   5,  9,   6,  9,   7,  9,   8,  9,   9,  9,  10,  9,  11,  9,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,  17,  9,  18,  9,  19,  9,  20,  9,  21,  9,  22,  9,  23,  9,  24,  9,  25,  9,  26,  9,  27,  9,  28,  9,  29,  9,   6,  5,   0,  4,  6,  5,  6,  5,  6,  5,
            0, 8,   5, 3,   0, 4,   6, 5,   6, 5,   4, 10,   5, 10,   6, 10,   7, 10,   8, 10,   9, 10,  10, 10,  11, 10,   6,  5,   6,  5,   8,  0,  15, 10,  16, 10,  17, 10,  18, 10,  19, 10,  20, 10,  21, 10,  22, 10,  23, 10,  24, 10,  25, 10,  26, 10,  27, 10,  28, 10,  29, 10,   6,  5,   0,  4,  6,  5,  6,  5,  6,  5,
            0, 8,   5, 3,   0, 4,   6, 5,   6, 5,   4, 11,   5, 11,   6, 11,   7, 11,   8, 11,   9, 11,  10, 11,  11, 11,   6,  5,   6,  5,  14, 11,  15, 11,  16, 11,  17, 11,  18, 11,  19, 11,  20, 11,  21, 11,  22, 11,  23, 11,  24, 11,  25, 11,  26, 11,  27, 11,  28, 11,  29, 11,   6,  5,   0,  4,  6,  5,  6,  5,  6,  5,
            0, 8,   5, 3,   0, 4,   6, 5,   6, 5,   4, 12,   5, 12,   6, 12,   7, 12,   8, 12,   9, 12,  10, 12,  11, 12,   6,  5,   6,  5,  14, 12,  15, 12,  16, 12,  17, 12,  18, 12,  19, 12,  20, 12,  21, 12,  22, 12,  23, 12,  24, 12,  25, 12,  26, 12,  27, 12,  28, 12,  29, 12,   6,  5,   0,  4,  6,  5,  6,  5,  6,  5,
            0, 8,   5, 3,   0, 4,   6, 5,   6, 5,   4, 13,   5, 13,   6, 13,   7, 13,   8, 13,   9, 13,  10, 13,  11, 13,   6,  5,   6,  5,   6,  5,  15, 13,  16, 13,  17, 13,  18, 13,  19, 13,  20, 13,  21, 13,  22, 13,  23, 13,  24, 13,  25, 13,  26, 13,  27, 13,  28, 13,  29, 13,   6,  5,   0,  4,  6,  5,  6,  5,  6,  5,
            0, 8,   5, 3,   0, 4,   6, 5,   6, 5,   4, 14,   5, 14,   6, 14,   7, 14,   8, 14,   9, 14,  10, 14,  11, 14,  12, 14,  13, 14,   6,  5,   6,  5,   6,  5,  17, 14,   5, 16,   5, 16,  20, 14,   6,  5,  22, 14,  23, 14,  24, 14,  25, 14,  26, 14,   5, 16,   5, 16,  29, 14,   6,  5,   0,  4,  6,  5,  6,  5,  6,  5,
            0, 8,   5, 3,   0, 4,   6, 5,   6, 5,   4, 15,   5, 15,   6, 15,   7, 15,   8, 15,   9, 15,  10, 15,  11, 15,  12, 15,  13, 15,   8,  0,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,  20, 15,   5, 16,   5, 16,   5, 16,   5, 16,   5, 16,   4, 15,   6,  5,   6,  5,   6,  5,   6,  5,   0,  4,  6,  5,  6,  5,  6,  5,
            0, 8,   5, 3,   0, 4,   6, 5,   6, 5,   4, 16,   5, 16,   6, 16,   7, 16,   8, 16,   9, 16,  10, 16,  11, 16,  12, 16,  13, 16,   6,  5,   6,  5,   6,  5,   0,  4,   0,  4,   0,  4,   0,  4,   0,  4,   0,  4,   0,  4,   0,  4,   0,  4,   0,  4,   0,  4,   0,  4,   0,  4,   0,  4,   0,  4,  6,  5,  6,  5,  6,  5,
            0, 8,   5, 3,   0, 4,   0, 4,   0, 4,   0,  4,   0,  4,   0,  4,   0,  4,   0,  4,   0,  4,   0,  4,   0,  4,   0,  4,   0,  4,   0,  4,   0,  4,   0,  4,   0,  4,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,  24, 16,  25, 16,  26, 16,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,  6,  5,  6,  5,  6,  5,
            0, 8,   5, 3,   6, 5,   6, 5,   6, 5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,  24, 17,  25, 17,  26, 17,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,  6,  5,  6,  5,  6,  5,
            0, 8,   5, 3,   6, 5,   6, 5,   6, 5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,  6,  5,  6,  5,  6,  5,
            0, 8,   5, 3,   6, 5,   6, 5,   6, 5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,  6,  5,  6,  5,  6,  5,
            0, 8,   5, 3,   6, 5,   6, 5,   6, 5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,   6,  5,  6,  5,  6,  5,  6,  5}}}
