local camera <const> = require "nngn.lib.camera"
local entity <const> = require "nngn.lib.entity"
local input <const> = require "nngn.lib.input"
local state <const> = require "nngn.lib.state"
local utils <const> = require "nngn.lib.utils"

local IS_LOWER <const> = 0x20
assert(string.byte("a") == string.byte("A") | IS_LOWER)

local PIECE_PAWN <const> = string.byte("P")
local PIECE_ROOK <const> = string.byte("R")
local PIECE_KNIGHT <const> = string.byte("N")
local PIECE_BISHOP <const> = string.byte("B")
local PIECE_QUEEN <const> = string.byte("Q")
local PIECE_KING <const> = string.byte("K")

local MOVE <const> = 1 << 0
local CAPTURE <const> = 1 << 1

local board_init <const> =
    "rnbqkbnr" ..
    "pppppppp" ..
    "        " ..
    "        " ..
    "        " ..
    "        " ..
    "PPPPPPPP" ..
    "RNBQKBNR"

local move_renderer <const> = {
    type = Renderer.SPRITE,
    tex = 1,
    size = {28, 28},
    z_off = 0,
    scale = {512, 512},
    coords = {1, 2},
}

local capture_renderer <const> = {
    type = Renderer.SPRITE,
    tex = 1,
    size = {28, 28},
    z_off = 0,
    scale = {512, 512},
    coords = {0, 2},
}

local function restore_state(self)
    state.restore(self.state)
    nngn:input():set_binding_group(input.input)
    self.state = nil
end

local function same_color(p0, p1)
    return ((p0.type ~ p1.type) & IS_LOWER) == 0
end

local function valid_pos(x, y)
    return 0 <= x and x <= 7 and 0 <= y and y <= 7
end

local function piece_at(self, x, y)
    assert(valid_pos(x, y))
    return self.board[8 * y + x]
end

local function world_pos(self, x, y)
    return
        self.x0 + 4 * self.cell_size * x,
        self.y0 + 4 * self.cell_size * y
end

local add_move
local function moves_for(self, p0, x, y)
    local type <const> = p0.type & ~IS_LOWER
    local ret <const> = {}
    if type == PIECE_PAWN then
        if p0.type & IS_LOWER == 0 then
            add_move(self, ret, p0, x - 1, y + 1, CAPTURE)
            add_move(self, ret, p0, x + 1, y + 1, CAPTURE)
            if add_move(self, ret, p0, x, y + 1, MOVE) and y == 1 then
                add_move(self, ret, p0, x, y + 2, MOVE)
            end
        else
            add_move(self, ret, p0, x - 1, y - 1, CAPTURE)
            add_move(self, ret, p0, x + 1, y - 1, CAPTURE)
            if add_move(self, ret, p0, x, y - 1, MOVE) and y == 6 then
                add_move(self, ret, p0, x, y - 2, MOVE)
            end
        end
    elseif type == PIECE_KNIGHT then
        add_move(self, ret, p0, x + 1, y + 2)
        add_move(self, ret, p0, x + 2, y + 1)
        add_move(self, ret, p0, x + 2, y - 1)
        add_move(self, ret, p0, x + 1, y - 2)
        add_move(self, ret, p0, x - 1, y - 2)
        add_move(self, ret, p0, x - 2, y - 1)
        add_move(self, ret, p0, x - 2, y + 1)
        add_move(self, ret, p0, x - 1, y + 2)
    elseif type == PIECE_KING then
        for ix = -1, 1 do
            for iy = -1, 1 do
                add_move(self, ret, p0, x + ix, y + iy)
            end
        end
    else
        if type == PIECE_ROOK or type == PIECE_QUEEN then
            for x = x - 1, 0, -1 do
                if not add_move(self, ret, p0, x, y) then break end
            end
            for x = x + 1, 7     do
                if not add_move(self, ret, p0, x, y) then break end
            end
            for y = y - 1, 0, -1 do
                if not add_move(self, ret, p0, x, y) then break end
            end
            for y = y + 1, 7     do
                if not add_move(self, ret, p0, x, y) then break end
            end
        end
        if type == PIECE_BISHOP or type == PIECE_QUEEN then
            for i = 1, 7 do
                if not add_move(self, ret, p0, x - i, y - i) then break end
            end
            for i = 1, 7 do
                if not add_move(self, ret, p0, x + i, y - i) then break end
            end
            for i = 1, 7 do
                if not add_move(self, ret, p0, x - i, y + i) then break end
            end
            for i = 1, 7 do
                if not add_move(self, ret, p0, x + i, y + i) then break end
            end
        end
    end
    return ret
end

function add_move(self, t, p0, x, y, flags)
    flags = flags or (MOVE | CAPTURE)
    if not valid_pos(x, y) then
        return false
    end
    local p1 <const> = piece_at(self, x, y)
    if p1 then
        if flags & CAPTURE ~= 0 and not same_color(p0, p1) then
            table.insert(t, {x, y, capture_renderer})
        end
    elseif flags & MOVE ~= 0 then
        table.insert(t, {x, y, move_renderer})
        return true
    end
    return false
end

local function move_cursor(self, x, y)
    local c <const> = self.tmp_cursor or self:get_cursors()
    x = c.pos[1] + x
    y = c.pos[2] + y
    if not valid_pos(x, y) then
        return
    end
    c.pos[1] = x
    c.pos[2] = y
    x, y = world_pos(self, x, y)
    c.entity:set_pos(x, y, 0)
end

local function capture(self, x0, y0, x1, y1, p0, p1, i)
    if same_color(p0, p1) then
        error(string.format(
            "source (%d, %d) and destination (%d, %d) have the same color",
            x0, y0, x1, y1))
    end
    nngn:remove_entity(p1.entity)
    self.board[i] = nil
end

local function move(self, x0, y0, x1, y1)
    local i0 <const> = 8 * y0 + x0
    local i1 <const> = 8 * y1 + x1
    local t <const> = self.board
    local p0 <const> = t[i0]
    if not p0 then
        error(string.format("invalid source position: %d %d", x0, y0))
    end
    local p1 <const> = t[i1]
    if p1 then
        capture(self, x0, y0, x1, y1, p0, p1, i1)
    end
    t[i0] = nil
    t[i1] = p0
    p0.pos[1] = x1
    p0.pos[2] = y1
    x1, y1 = world_pos(self, x1, y1)
    p0.entity:set_pos(x1, y1, 0)
end

local function init_move(self)
    local c <const> = self:get_cursors()
    local pos <const> = c.pos
    local x <const>, y <const> = table.unpack(pos)
    local p <const> = piece_at(self, table.unpack(pos))
    if not p then
        return
    end
    self.tmp_cursor = {
        pos = {x, y},
        entity = entity.load(nil, "src/lson/cursor.lua", {
            pos = {world_pos(self, x, y)},
        }),
    }
    for _, t in ipairs(moves_for(self, p, x, y)) do
        local x <const>, y <const>, r <const> = table.unpack(t)
        table.insert(self.valid_moves, entity.load(nil, nil, {
            pos = {world_pos(self, x, y)},
            renderer = r,
        }))
    end
end

local function finish_move(self)
    local c0 <const> = self:get_cursors()
    local c1 <const> = self.tmp_cursor
    local p0 <const>, p1 <const> = c0.pos, c1.pos
    local ret = false
    if p0[1] ~= p1[1] or p0[2] ~= p1[2] then
        ret = true
        move(self, p0[1], p0[2], c1.pos[1], c1.pos[2])
    end
    c0.pos = p1
    c0.entity:set_pos(c1.entity:pos())
    nngn:remove_entity(c1.entity)
    self.tmp_cursor = nil
    for _, x in ipairs(self.valid_moves) do
        nngn:remove_entity(x)
    end
    self.valid_moves = {}
    return ret
end

local function next_turn(self)
    local c0 <const>, c1 <const> = self:get_cursors()
    c1.entity, c0.entity = c0.entity, nil
    c1.entity:set_pos(world_pos(self, table.unpack(c1.pos)))
end

local function action(self)
    if self.tmp_cursor then
        if finish_move(self) then
            next_turn(self)
        end
    else
        init_move(self)
    end
end

local function init_board(self)
    local size <const> = 4 * self.cell_size
    local r0 <const>, r1 <const> =
        utils.deep_copy_values(self.cell_renderer.renderer),
        utils.deep_copy_values(self.cell_renderer.renderer)
    r0.size = {size, size}
    r1.size = {size, size}
    r1.coords = {3, 1}
    r0.z_off = 16
    r1.z_off = 16
    for y = 0, 7 do
        for x = 0, 7 do
            table.insert(self.entities, entity.load(nil, nil, {
                pos = {world_pos(self, x, y)},
                renderer = (((x + (y & 1)) & 1) == 0) and r1 or r0,
            }))
        end
    end
end

local function init_pieces(self)
    local size <const> = 4 * self.cell_size
    local coords <const> = {k = 0, q = 1, b = 2, n = 3, r = 4, p = 5}
    local r <const> = {
        type = Renderer.TRANSLUCENT,
        tex = "img/chess.png",
        z_off = -12,
        size = {size, size},
        scale = {512//64, 512//64},
    }
    local space <const> = string.byte(" ")
    for y = 0, 7 do
        for x = 0, 7 do
            local i <const> = 1 + 8 * (7 - y) + x
            local s <const> = board_init:sub(i, i)
            local c <const> = string.byte(s)
            if c == space then
                goto continue
            end
            r.coords = {
                coords[string.lower(s)],
                (c < string.byte("a")) and 1 or 0,
            }
            local e <const> = entity.load(nil, nil, {
                pos = {world_pos(self, x, y)},
                renderer = r,
            })
            table.insert(self.entities, e)
            local p <const> = {type = c, pos = {x, y}, entity = e}
            table.insert(self.pieces, p)
            self.board[8 * y + x] = p
            ::continue::
        end
    end
end

local function init_cursor(self)
    self.cursor0.entity = entity.load(nil, "src/lson/cursor.lua", {
        pos = {world_pos(self, table.unpack(self.cursor0.pos))},
    })
end

local function init_input(self)
    if not self.input then
        local i <const> = BindingGroup.new()
        local function f(k, f) i:add(k, Input.SEL_PRESS, f) end
        f(Input.KEY_ESC, function() nngn:exit() end)
        f(string.byte("Q"), function() restore_state(self) end)
        f(string.byte("A"), function() move_cursor(self, -1, 0) end)
        f(string.byte("D"), function() move_cursor(self, 1, 0) end)
        f(string.byte("S"), function() move_cursor(self, 0, -1) end)
        f(string.byte("W"), function() move_cursor(self, 0, 1) end)
        f(string.byte(" "), function() action(self) end)
        self.input = i
    end
    nngn:input():set_binding_group(self.input)
end

local function reset(self)
    if self.tmp_cursor then
        nngn:remove_entity(self.tmp_cursor.entity)
        self.tmp_cursor = nil
    end
    local cursor <const> = self.cursor0.entity or self.cursor1.entity
    if cursor then
        nngn:remove_entity(cursor)
        self.cursor0.entity = nil
        self.cursor1.entity = nil
    end
    for _, x in ipairs(self.valid_moves) do
        nngn:remove_entity(x)
    end
    self.valid_moves = {}
    if self.state then
        state.restore(self.state)
        self.state = nil
    end
    for _, x in ipairs(self.entities) do
        nngn:remove_entity(x)
    end
    self.entities = {}
    nngn:input():set_binding_group(input.input)
end

local chess <const> = {}
chess.__index = chess

function chess:new()
    return setmetatable({
        entities = {},
        x0 = 0,
        y0 = 0,
        timer_ms = 0,
        board = {},
        pieces = {},
        cursor0 = {pos = {4, 0}},
        cursor1 = {pos = {4, 7}},
        valid_moves = {},
    }, self)
end

function chess:get_cursors()
    if self.cursor1.entity then
        return self.cursor1, self.cursor0
    else
        return self.cursor0, self.cursor1
    end
end

function chess:init(cell_size, cell_renderer, x0, y0)
    self.cell_size, self.cell_renderer, self.x0, self.y0 =
        cell_size, cell_renderer, x0, y0
    init_board(self)
    init_pieces(self)
end

function chess:restart()
    self:start()
end

function chess:reset()
    reset(self)
end

function chess:start()
    init_input(self)
    if not (self.cursor0.entity or self.cursor1.entity) then
        init_cursor(self)
    end
    self.state = state.save{camera = true}
    local c <const> = camera.get()
    c:set_pos(-192, -192, 0)
    c:set_zoom(2)
end

function chess:update(dt_ms)
    self.timer_ms = self.timer_ms + dt_ms
    if self.timer_ms < 1000 then
        return
    end
    self.timer_ms = self.timer_ms - 1000
end

return chess
