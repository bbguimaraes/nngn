local camera = require "nngn.lib.camera"
local entity = require "nngn.lib.entity"
local map = require "nngn.lib.map"
local player = require "nngn.lib.player"
local texture = require "nngn.lib.texture"

local entities = {}
local warp
local stars = {}
local cell_size = 8
local cell_renderer = {
    renderer = {
        type = Renderer.SPRITE, tex = 1,
        size = {cell_size - 2, cell_size - 2},
        scale = {512, 512}, z_off = cell_size}}
local star_renderer = dofile("src/lson/star.lua")
local conway = {started = false, size = 64, tick_ms = 250, timer_ms = 0}
local maze = {size = 32, tick_ms = 50, timer_ms = 0}
local info_stars = {
    function() conway.started = true end,
    function() if #maze.stack == 0 then maze:reset() maze:start() end end}

function conway:init(pos_off, board_str)
    local size = self.size
    self.n = size * size
    local board = {}
    for i = 0, size - 1 do board[i] = {} end
    for i = 0, self.n - 1 do
        local x, y = i % size, i // size
        local e = entity.load(nil, nil, {
            pos = {
                pos_off[1] + cell_size * (0.5 + x),
                pos_off[2] + cell_size * (0.5 + y), 0}})
        if board_str:sub(i + 1, i + 1) ~= " " then
            entity.load(e, nil, cell_renderer)
        end
        board[y][x] = e
        table.insert(entities, e)
    end
    self.board = board
end

function conway:update(dt_ms)
    if not self.started then return end
    self.timer_ms = self.timer_ms + dt_ms
    while self.timer_ms > self.tick_ms do
        self.timer_ms = self.timer_ms - self.tick_ms
        self:next_generation()
    end
end

function conway:next_generation()
    local adj = self:gen_adjacent()
    local new, kill = self:gen_actions(adj)
    self.gen(new, kill)
    self.kill(kill)
end

function conway:gen_adjacent()
    local renderer = Entity.renderer
    local size, board = self.size, self.board
    local c = function(x, y)
        if x < 0 or size <= x or y < 0 or size <= y
            or not renderer(board[y][x])
        then return 0
        else return 1 end
    end
    local ret = {}
    for i = 0, size - 1 do ret[i] = {} end
    for y = 0, size - 1 do
        for x = 0, size - 1 do
            ret[y][x] =
                c(x - 1, y - 1) + c(x, y - 1) + c(x + 1, y - 1) +
                c(x - 1, y    )               + c(x + 1, y    ) +
                c(x - 1, y + 1) + c(x, y + 1) + c(x + 1, y + 1)
        end
    end
    return ret
end

function conway:gen_actions(adj)
    local renderer = Entity.renderer
    local size, board = self.size, self.board
    local new, kill = {}, {}
    for y = 0, size - 1 do
        for x = 0, size - 1 do
            local e = board[y][x]
            local r = renderer(e)
            local n_adj = adj[y][x]
            if n_adj == 3 then
                if not r then table.insert(new, {x, y, e}) end
            elseif n_adj ~= 2 and r then
                table.insert(kill, {x, y, e})
            end
        end
    end
    return new, kill
end

function conway.gen(new, kill)
    local load = entity.load
    local renderer = Entity.renderer
    local set_renderer = Entity.set_renderer
    for _, t in ipairs(new) do
        local x, y, e = table.unpack(t)
        local k = table.remove(kill)
        if k then
            set_renderer(e, renderer(k[3]))
            set_renderer(k[3], nil)
        else
            load(e, nil, cell_renderer)
        end
    end
end

function conway.kill(kill)
    local renderers = nngn.renderers
    local remove = renderers.remove
    local renderer = Entity.renderer
    local set_renderer = Entity.set_renderer
    for _, t in ipairs(kill) do
        local x, y, e = table.unpack(t)
        remove(renderers, renderer(e))
        set_renderer(e, nil)
    end
end

function maze:init(pos_off)
    self.stack = {}
    self.max_idx = 2 * self.size - 1
    local board = {}
    for y = 0, self.max_idx + 1 do
        local row = {}
        for x = 0, self.max_idx + 1 do
            local e = entity.load(nil, nil, {
                pos = {
                    pos_off[1] + cell_size * (0.5 + x),
                    pos_off[2] + cell_size * (0.5 + y), 0},
                renderer = cell_renderer.renderer})
            table.insert(entities, e)
            row[x] = {entity = e, visited = false}
        end
        board[y] = row
    end
    self.board = board
end

function maze:reset()
    for _, y in pairs(self.board) do
        for _, x in pairs(y) do
            x.visited = false
            local e = x.entity
            if not e:renderer() then
                entity.load(e, nil, cell_renderer)
            end
        end
    end
end

function maze:start() table.insert(self.stack, {1, 1}) end

function maze:pop()
    self:clear_pos(table.unpack(table.remove(self.stack)))
end

function maze:clear_pos(x, y)
    local e = self.board[y][x].entity
    nngn.renderers:remove(e:renderer())
    e:set_renderer(nil)
end

function maze:update(dt_ms)
    if #self.stack == 0 then return end
    self.timer_ms = self.timer_ms + dt_ms
    while self.timer_ms > self.tick_ms do
        self.timer_ms = self.timer_ms - self.tick_ms
        self:next_generation()
        if #self.stack == 0 then break end
    end
end

function maze:next_generation()
    local min, max, board = 2, self.max_idx, self.board
    local x, y = table.unpack(self.stack[#self.stack])
    local b = board[y][x]
    b.visited = true
    entity.load(b.entity, nil, star_renderer)
    local adj = {}
    if min < x then
        local b = board[y][x - 2]
        if not b.visited then table.insert(adj, {-1,  0, b}) end
    end
    if x < max then
        local b = board[y][x + 2]
        if not b.visited then table.insert(adj, { 1,  0, b}) end
    end
    if min < y then
        local b = board[y - 2][x]
        if not b.visited then table.insert(adj, { 0, -1, b}) end
    end
    if y < max then
        local b = board[y + 2][x]
        if not b.visited then table.insert(adj, { 0,  1, b}) end
    end
    if #adj == 0 then return self:pop() end
    local nx, ny, b = table.unpack(adj[nngn.math:rand_int(#adj)])
    self:clear_pos(x + nx, y + ny)
    table.insert(self.stack, {x + 2 * nx, y + 2 * ny})
end

local function init()
    -- warp
    do
        local ts = dofile("src/lson/pillar.lua")
        ts[1].pos = {0, -32}
        ts[2].pos = {0, -32}
        ts[1].collider = {
            type = Collider.AABB, bb = {-2, -10, 2, -6},
            flags = Collider.TRIGGER}
        table.insert(entities, entity.load(nil, nil, ts[1]))
        warp = entities[#entities]
        table.insert(entities, entity.load(nil, nil, ts[2]))
    end
    local coll_st = Collider.SOLID | Collider.TRIGGER
    -- stars
    for _, p in ipairs({{32, 32}, {-32, 32}}) do
        local e = entity.load(nil, nil, {
            pos = p, renderer = star_renderer.renderer,
            collider = {
                type = Collider.AABB,
                bb = 16, m = Math.INFINITY, flags = coll_st}})
        table.insert(entities, e)
        table.insert(stars, e)
    end
    conway:init({64, 64},
        " o                                                            oo" ..
        "  o                                         ooo   ooo         oo" ..
        "ooo                                                         oo  " ..
        "                                          o    o o    o     oo  " ..
        "                                          o    o o    o         " ..
        "                                          o    o o    o         " ..
        "                                            ooo   ooo           " ..
        "                                                                " ..
        "                                            ooo   ooo           " ..
        "                                          o    o o    o         " ..
        "                                          o    o o    o     o   " ..
        "                                          o    o o    o    ooo  " ..
        "                                                          ooooo " ..
        "                                            ooo   ooo           " ..
        "                                                                " ..
        "                                                                " ..
        "                                                                " ..
        "                                                                " ..
        "                                                                " ..
        "                                                          ooooo " ..
        "                                                           ooo  " ..
        "                                                            o   " ..
        "                                                                " ..
        "                                                                " ..
        "                                                                " ..
        "                                                                " ..
        "                                                             oo " ..
        "                                                            o  o" ..
        "                                                             oo " ..
        "                                                                " ..
        "  oo                                                            " ..
        "oo oo                                                           " ..
        "oooo                                                            " ..
        " oo                                                           o " ..
        "                                                             o o" ..
        "                                                            o  o" ..
        "                                                             oo " ..
        "                                                                " ..
        "                                                                " ..
        "                                                                " ..
        "                                                                " ..
        "                                                                " ..
        "                                                            o   " ..
        "                                                           ooo  " ..
        "                                                          ooooo " ..
        "                                                                " ..
        "                        o                                       " ..
        "                      o o                                       " ..
        "            oo      oo            oo                            " ..
        "           o   o    oo            oo                            " ..
        "oo        o     o   oo                                          " ..
        "oo        o   o oo    o o                                 ooooo " ..
        "          o     o       o                                  ooo  " ..
        "           o   o                                            o   " ..
        "            oo                                                  " ..
        "                                                                " ..
        "                                                                " ..
        "                                                                " ..
        "                                                                " ..
        "                                                                " ..
        "  oo                                                        oo  " ..
        "  oo       ooo    o    o                o    o    ooo       oo  " ..
        "oo    ooo   ooo  o o  o o              o o  o o  ooo   ooo    oo" ..
        "oo                o   oo                oo   o                oo")
    maze:init({-584, 64})
    player.move_all(0, 32, true)
    nngn.camera:set_zoom(.5)
    nngn.camera:set_pos(0, 256, 0)
    camera.set_follow(nil)
end

local function heartbeat()
    local dt_ms = nngn.timing:dt_ms()
    conway:update(dt_ms)
    maze:update(dt_ms)
end

local function on_collision(e0, e1)
    if e0 == warp or e1 == warp then map.next("maps/main.lua") end
    for i, e in ipairs(stars) do
        if e0 == e or e1 == e then
            return info_stars[i]()
        end
    end
end

map.map {
    name = "conway",
    file = "maps/conway.lua",
    state = {camera_follow = true},
    init = init,
    entities = entities,
    reset = camera.reset,
    heartbeat = heartbeat,
    on_collision = on_collision,
    tiles = {0, 1, 0, 0, 1, 1, 0, 0, {}},
}
