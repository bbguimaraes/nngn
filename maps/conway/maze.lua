local entity <const> = require "nngn.lib.entity"

local function clear_pos(self, x, y)
    local e = self.board[y][x].entity
    nngn:renderers():remove(e:renderer())
    e:set_renderer(nil)
end

local function pop(self)
    clear_pos(self, table.unpack(table.remove(self.stack)))
end

local function next_generation(self)
    local min, max, board = 2, self.max_idx, self.board
    local x, y = table.unpack(self.stack[#self.stack])
    local b = board[y][x]
    b.visited = true
    entity.load(b.entity, nil, self.star_renderer)
    local adj = {}
    if min < x then
        local b = board[y][x - 2]
        if not b.visited then
            table.insert(adj, {-1,  0, b})
        end
    end
    if x < max then
        local b = board[y][x + 2]
        if not b.visited then
            table.insert(adj, { 1,  0, b})
        end
    end
    if min < y then
        local b = board[y - 2][x]
        if not b.visited then
            table.insert(adj, { 0, -1, b})
        end
    end
    if y < max then
        local b = board[y + 2][x]
        if not b.visited then
            table.insert(adj, { 0,  1, b})
        end
    end
    if #adj == 0 then
        return pop(self)
    end
    local nx, ny, b = table.unpack(adj[nngn:math():rand_int(#adj)])
    clear_pos(self, x + nx, y + ny)
    table.insert(self.stack, {x + 2 * nx, y + 2 * ny})
end

local function reset(self)
    for _, y in pairs(self.board) do
        for _, x in pairs(y) do
            x.visited = false
            local e <const> = x.entity
            if not e:renderer() then
                entity.load(e, nil, cell_renderer)
            end
        end
    end
end

local maze <const> = {}
maze.__index = maze

function maze:new()
    return setmetatable({
        entities = {},
        size = 32,
        tick_ms = 50,
        timer_ms = 0,
    }, self)
end

function maze:done() return #self.stack == 0 end
function maze:start() table.insert(self.stack, {1, 1}) end

function maze:restart()
    if self:done() then
        reset(self)
        self:start()
    end
end

function maze:init(cell_size, cell_renderer, star_renderer, pos_off)
    self.stack = {}
    self.max_idx = 2 * self.size - 1
    self.star_renderer = star_renderer
    local board <const> = {}
    for y = 0, self.max_idx + 1 do
        local row <const> = {}
        for x = 0, self.max_idx + 1 do
            local e <const> = entity.load(nil, nil, {
                pos = {
                    pos_off[1] + cell_size * (0.5 + x),
                    pos_off[2] + cell_size * (0.5 + y),
                    0,
                },
                renderer = cell_renderer.renderer,
            })
            row[x] = {entity = e, visited = false}
        end
        board[y] = row
    end
    self.board = board
end

function maze:reset()
    for _, y in pairs(self.board) do
        for _, x in pairs(y) do
            nngn:remove_entity(x.entity)
        end
    end
    self.board = {}
end

function maze:update(dt_ms)
    if #self.stack == 0 then
        return
    end
    local t = self.timer_ms + dt_ms
    local tick <const> = self.tick_ms
    while #self.stack ~= 0 and tick < t do
        t = t - tick
        next_generation(self)
    end
    self.timer_ms = t
end

return maze
