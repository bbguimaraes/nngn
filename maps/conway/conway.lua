local entity <const> = require "nngn.lib.entity"

local board_init <const> =
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
    "oo                o   oo                oo   o                oo"

local gen_adjacent, gen_actions, gen, kill
local function next_generation(self)
    local adj <const> = gen_adjacent(self)
    local new <const>, k <const> = gen_actions(self, adj)
    gen(self, new, k)
    kill(k)
end

local alive
function gen_adjacent(self)
    local renderer <const> = Entity.renderer
    local size <const>, board <const> = self.size, self.board
    local ret <const> = {}
    for i = 0, size - 1 do
        ret[i] = {}
    end
    for y = 0, size - 1 do
        for x = 0, size - 1 do
            ret[y][x] =
                  alive(board, size, renderer, x - 1, y - 1)
                + alive(board, size, renderer, x,     y - 1)
                + alive(board, size, renderer, x + 1, y - 1)
                + alive(board, size, renderer, x - 1, y    )
                + alive(board, size, renderer, x + 1, y    )
                + alive(board, size, renderer, x - 1, y + 1)
                + alive(board, size, renderer, x,     y + 1)
                + alive(board, size, renderer, x + 1, y + 1)
        end
    end
    return ret
end

function alive(board, size, r, x, y)
    if x < 0 or size <= x or y < 0 or size <= y then
        return 0
    end
    if not r(board[y][x]) then
        return 0
    end
    return 1
end

function gen_actions(self, adj)
    local renderer <const> = Entity.renderer
    local size <const>, board <const> = self.size, self.board
    local new <const>, kill <const> = {}, {}
    for y = 0, size - 1 do
        for x = 0, size - 1 do
            local e <const> = board[y][x]
            local r <const> = renderer(e)
            local n_adj <const> = adj[y][x]
            if n_adj == 3 then
                if not r then
                    table.insert(new, {x, y, e})
                end
            elseif n_adj ~= 2 and r then
                table.insert(kill, {x, y, e})
            end
        end
    end
    return new, kill
end

function gen(self, new, kill)
    local load <const> = entity.load
    local renderer <const> = Entity.renderer
    local set_renderer <const> = Entity.set_renderer
    for _, t in ipairs(new) do
        local x <const>, y <const>, e <const> = table.unpack(t)
        local k <const> = table.remove(kill)
        if k then
            set_renderer(e, renderer(k[3]))
            set_renderer(k[3], nil)
        else
            load(e, nil, self.cell_renderer)
        end
    end
end

function kill(kill)
    local renderers <const> = nngn:renderers()
    local remove <const> = renderers.remove
    local renderer <const> = Entity.renderer
    local set_renderer <const> = Entity.set_renderer
    for _, t in ipairs(kill) do
        local x <const>, y <const>, e <const> = table.unpack(t)
        remove(renderers, renderer(e))
        set_renderer(e, nil)
    end
end

local conway <const> = {}
conway.__index = conway

function conway:new()
    return setmetatable({
        started = false,
        size = 64,
        tick_ms = 250,
        timer_ms = 0,
    }, self)
end

function conway:start() self.started = true end
function conway:restart() self:start() end

function conway:init(cell_size, cell_renderer, pos_off)
    local size = self.size
    self.cell_renderer, self.n = cell_renderer, size * size
    local board = {}
    for i = 0, size - 1 do board[i] = {} end
    for i = 0, self.n - 1 do
        local x, y = i % size, i // size
        local t <const> = {
            pos = {
                pos_off[1] + cell_size * (0.5 + x),
                pos_off[2] + cell_size * (0.5 + y),
                0,
            },
        }
        if board_init:sub(i + 1, i + 1) ~= " " then
            t.renderer = cell_renderer.renderer
        end
        local e = entity.load(nil, nil, t)
        board[y][x] = e
    end
    self.board = board
end

function conway:reset()
    local board <const>, size <const> = self.board, self.size
    for y = 0, size - 1 do
        local b <const> = board[y]
        for x = 0, size - 1 do
            nngn:remove_entity(b[x])
        end
    end
    self.board = nil
end

function conway:update(dt_ms)
    if not self.started then
        return
    end
    self.timer_ms = self.timer_ms + dt_ms
    while self.tick_ms < self.timer_ms do
        self.timer_ms = self.timer_ms - self.tick_ms
        next_generation(self)
    end
end

return conway
