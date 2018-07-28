local entity = require "nngn.lib.entity"
local input = require "nngn.lib.input"
local map = require "nngn.lib.map"
local player = require "nngn.lib.player"
local state = require "nngn.lib.state"
local textbox = require "nngn.lib.textbox"

local vec3 <const> = require("nngn.lib.math").vec3

local GRID_SIZE <const> = 32
local entities = {}
local warps, trigger

function F(f)
    return math.tointeger(math.ceil(f * 1000 / 59.73))
end

local PLAYERS <const> = {
    LYN = 0,
    ELIWOOD = 1,
}

local MOVE_SPRITE <const> = {
    renderer = {
        type = Renderer.SPRITE, tex = "img/fire_emblem/0.png",
        size = {32, 32}, z_off = 16,
    },
    anim = {sprite = {512//16, 512//16, {{
        { 0, 0, F(4)}, { 1, 0, F(4)}, { 2, 0, F(4)}, { 3, 0, F(4)},
        { 4, 0, F(4)}, { 5, 0, F(4)}, { 6, 0, F(4)}, { 7, 0, F(4)},
        { 8, 0, F(4)}, { 9, 0, F(4)}, {10, 0, F(4)}, {11, 0, F(4)},
        {12, 0, F(4)}, {13, 0, F(4)}, {14, 0, F(4)}, {15, 0, F(4)},
    }}}},
}

local CURSOR_SPRITE <const> =
    dofile("src/lson/fire_emblem/cursor.lua")
local CURSOR_SPRITE_ANIM <const> =
    dofile("src/lson/fire_emblem/cursor_anim.lua")

local HIGHLIGHT_SPRITE <const> = {
    renderer = {
        type = Renderer.SPRITE, tex = "img/fire_emblem/0.png",
        size = {64, 64}, z_off = -16,
    },
    anim = {sprite = {512//32, 512//32, {{
        { 8, 1, F(8)}, { 9, 1, F(2)}, {10, 1, F(2)},
        {11, 1, F(2)}, {12, 1, F(2)}, {13, 1, F(6)},
        {13, 2, F(2)}, {12, 2, F(2)}, {12, 1, F(2)},
        {11, 2, F(2)}, {10, 2, F(2)}, { 9, 1, F(2)},
    }}}},
}

local FE <const> = {
    players = {},
    teams = {},
    move = {range = {}, path = {}, path_pos = {}},
    cursor_pos = {0, 0},
    cur_team = 0
}

function FE:init()
    self:_init_input()
    self:_init_entities()
    self:_init_players()
    self:next_turn()
end

function FE:start()
    if not self.input then
        self:init()
    end
    nngn:input():set_binding_group(self.input)
    self.state = state.save{camera = true}
    nngn:camera():set_pos(0, 0, 0)
end

function FE:_init_input()
    local i <const> = BindingGroup.new()
    i:add(Input.KEY_ESC, Input.SEL_PRESS, function() nngn:exit() end)
    i:add(string.byte("Q"), Input.SEL_PRESS, function()
        nngn:input():set_binding_group(self.prev_input)
        state.restore(self.state)
        self.state = nil
    end)
    i:add(string.byte("A"), Input.SEL_PRESS, function()
        self:move_cursor(-1, 0)
    end)
    i:add(string.byte("D"), Input.SEL_PRESS, function()
        self:move_cursor(1, 0)
    end)
    i:add(string.byte("S"), Input.SEL_PRESS, function()
        self:move_cursor(0, -1)
    end)
    i:add(string.byte("W"), Input.SEL_PRESS, function()
        self:move_cursor(0, 1)
    end)
    i:add(string.byte("F"), Input.SEL_PRESS, function()
        self:select()
    end)
    self.prev_input = nngn:input():binding_group()
    self.input = i
end

function FE:_init_entities()
    table.insert(entities, entity.load(nil, nil, {
        pos = {-16, 176},
        renderer = HIGHLIGHT_SPRITE.renderer,
        anim = HIGHLIGHT_SPRITE.anim,
    }))
    table.insert(entities, entity.load(nil, nil, {
        pos = {16, 176},
        renderer = HIGHLIGHT_SPRITE.renderer,
        anim = HIGHLIGHT_SPRITE.anim,
    }))
    self.cursor = entity.load(nil, nil, {
        pos = {16, 16},
        renderer = CURSOR_SPRITE.renderer,
        anim = CURSOR_SPRITE_ANIM,
    })
end

function FE:_init_players()
    table.insert(self.teams, {name = "team0"})
    table.insert(self.teams, {name = "team1"})
    self:set_player_anim(0, self:add_player(PLAYERS.LYN, {0, 0}, 1))
    self:set_player_anim(0, self:add_player(PLAYERS.ELIWOOD, {-1, 0}, 2))
end

function FE:reset()
    nngn:remove_entity(self.cursor)
    for _, x in ipairs(self.players) do
        nngn:remove_entity(x.e)
    end
    nngn:remove_entities(self.move.range)
    nngn:remove_entities(self.move.path)
    nngn:animations():remove(CURSOR_SPRITE_ANIM)
end

function FE:move_cursor(x, y)
    self.cursor_pos[1] = self.cursor_pos[1] + x
    self.cursor_pos[2] = self.cursor_pos[2] + y
    self.cursor:set_pos(table.unpack(
        vec3(self.cursor:pos()) + vec3(x, y) * vec3(GRID_SIZE)))
    if #self.move.range ~= 0 then
        self:update_move_path(x, y)
    end
end

function FE:add_player(name, pos, team)
    local t <const> = {
        pos = {self._world_pos(pos[1], pos[2])},
        renderer = {
            type = Renderer.SPRITE,
            tex = "img/fire_emblem/0.png",
            size = {64, 64}
        },
    }
    local ret <const> = {
        name = name,
        pos = pos,
        e = entity.load(nil, nil, t),
        move_range = 3,
        team = team,
    }
    table.insert(self.players, ret)
    return ret
end

function FE:set_player_anim(i, player)
    i = i * 3
    local y <const> = player.name + 2
    local f4 <const>, f32 <const> = F(4), F(32)
    entity.load(player.e, nil, {anim = {sprite = {
        512//32, 512//32,
        {{
            {i + 0, y, f32}, {i + 1, y, f4},
            {i + 2, y, f32}, {i + 1, y, f4},
        }},
    }}})
end

function FE:select()
    local pos <const> = self.cursor_pos
    local player
    for _, x in ipairs(self.players) do
        if x.pos[1] == pos[1] and x.pos[2] == pos[2] then
            player = x
            break
        end
    end
    if player then
        if #self.move.range == 0 then
            if player.team == self.cur_team then
                self:start_move(player)
            end
        else
            self:finish_move()
        end
    elseif #self.move.range ~= 0 then
        self:finish_move()
        self:next_turn()
    end
end

function FE:start_move(player)
    local pos <const> = vec3(table.unpack(player.pos))
    local size <const> = vec3(GRID_SIZE)
    local r <const> = player.move_range
    for y = -r, r do
        for x = -(r - math.abs(y)), r - math.abs(y) do
            table.insert(self.move.range, entity.load(nil, nil, {
                pos = {self._world_pos(pos[1] + x, pos[2] + y)},
                renderer = MOVE_SPRITE.renderer,
                anim = MOVE_SPRITE.anim,
            }))
        end
    end
    self:set_player_anim(1, player)
    self.move.player = player
    table.insert(self.move.path_pos, player.pos)
end

function FE:finish_move()
    local player <const> = self.move.player
    local d <const> = self._distance(self.cursor_pos, player.pos)
    if 0 < d and d <= player.move_range then
        player.pos[1] = self.cursor_pos[1]
        player.pos[2] = self.cursor_pos[2]
        local x, y <const> = self._world_pos(table.unpack(player.pos))
        player.e:set_pos(x, y, 0)
    end
    nngn:remove_entities(self.move.range)
    nngn:remove_entities(self.move.path)
    self:set_player_anim(0, player)
    self.move.range = {}
    self.move.path = {}
    self.move.path_pos = {}
end

function FE:update_move_path(x, y)
    local pos = self.cursor_pos
    local player <const> = self.move.player
    local p_pos <const> = player.pos
    if player.move_range < self._distance(pos, p_pos) then
        return
    end
    if pos[1] == p_pos[1] and pos[2] == p_pos[2] then
        nngn:remove_entities(self.move.path)
        self.move.path = {}
        self.move.path_pos = {{table.unpack(p_pos)}}
        return
    end
    for _, x in ipairs(self.move.path_pos) do
        if x[1] == pos[1] and x[2] == pos[2] then
            return
        end
    end
    nngn:remove_entities(self.move.path)
    self.move.path = {}
    table.insert(self.move.path_pos, {table.unpack(pos)})
    self:create_move_path(x, y)
end

function FE:create_move_path(x, y)
    local pos2 = self.cursor_pos
    local t <const> = {
        pos = {self._world_pos(table.unpack(pos2))},
        renderer = {
            type = Renderer.SPRITE, tex = "img/fire_emblem/0.png",
            size = {32, 32}, scale = {512//16, 512//16},
        },
    }
    -- XXX better organize tile sheet
    if x == -1 then
        t.renderer.coords = {7, 2}
    elseif x == 1 then
        t.renderer.coords = {6, 3}
    elseif y == -1 then
        t.renderer.coords = {7, 3}
    elseif y == 1 then
        t.renderer.coords = {6, 2}
    else
        error("invalid move: " .. x .. ", " .. y)
    end
    table.insert(self.move.path, entity.load(nil, nil, t))
    local n <const> = #self.move.path_pos
    for i = n - 1, 2, -1 do
        local pos1 <const> = self.move.path_pos[i]
        local pos0 <const> = self.move.path_pos[i - 1]
        t.pos = {self._world_pos(table.unpack(pos1))}
        x, y = pos2[1] - pos1[1], pos2[2] - pos1[2]
        local x0, y0 <const> = pos1[1] - pos0[1], pos1[2] - pos0[2]
        if x ~= 0 then
            if x == x0 then
                t.renderer.coords = {2, 2}
            elseif x == -1 then
                if y0 == -1 then
                    t.renderer.coords = {5, 2}
                else
                    t.renderer.coords = {5, 3}
                end
            else
                if y0 == -1 then
                    t.renderer.coords = {4, 2}
                else
                    t.renderer.coords = {4, 3}
                end
            end
        else
            if y == y0 then
                t.renderer.coords = {2, 3}
            elseif y == -1 then
                if x0 == -1 then
                    t.renderer.coords = {4, 3}
                else
                    t.renderer.coords = {5, 3}
                end
            else
                if x0 == -1 then
                    t.renderer.coords = {4, 2}
                else
                    t.renderer.coords = {5, 2}
                end
            end
        end
        pos2 = pos1
        table.insert(self.move.path, entity.load(nil, nil, t))
    end
    local pos1 <const> = self.move.path_pos[1]
    x, y = pos2[1] - pos1[1], pos2[2] - pos1[2]
    if x == -1 then
        t.renderer.coords = {1, 2}
    elseif x == 1 then
        t.renderer.coords = {0, 3}
    elseif y == -1 then
        t.renderer.coords = {1, 3}
    elseif y == 1 then
        t.renderer.coords = {0, 2}
    end
    t.pos = {self._world_pos(table.unpack(pos1))}
    table.insert(self.move.path, entity.load(nil, nil, t))
end

function FE:next_turn(t)
    local t <const> = self.cur_team % 2 + 1
    self.cur_team = t
    textbox.set(self.teams[t].name, " ")
end

function FE._world_pos(gx, gy)
    return GRID_SIZE * (gx + 0.5), GRID_SIZE * (gy + 0.5)
end

function FE._distance(p0, p1)
    return math.abs(p1[1] - p0[1]) + math.abs(p1[2] - p0[2])
end

local function init()
    -- warp
    warps = {}
    for _, t in ipairs({
        -- warps
        {{   0,  208}, { -32,  -16,  32,  16},  true, false},
        {{-208, -176}, { -16,  -16,  16,  16},  true, false},
        {{-272,  272}, { -16,  -16,  16,  16},  true, false},
        {{   0, -400}, {-352,  -16, 352,  16},  true, false},
        -- outer walls
        {{   0,  368}, {-192,  -16, 192,  16}, false, false},
        {{-368, -208}, { -16, -176,  16, 176}, false,  true},
        -- north rooms
        {{-208,  320}, { -16,  -32,  16,  32}, false,  true},
        {{-240,  268}, { -16,  -20,  16,  20}, false, false},
        {{ 256,  304}, { -32,  -16,  32,  16}, false, false},
        {{-304,  224}, { -16,  -32,  16,  32}, false,  true},
        {{-144,  272}, { -16,  -80,  16,  80}, false,  true},
        {{-256,  160}, { -32,  -32,  32,  32}, false,  true},
        {{-160,  160}, { -32,  -32,  32,  32}, false,  true},
        {{-304,   96}, { -16,  -32,  16,  32}, false,  true},
        -- west rooms
        {{-256,   48}, { -32,  -16,  32,  16}, false,  true},
        {{-176,   48}, { -16,  -16,  16,  16}, false,  true},
        {{-144,  -16}, { -16,  -80,  16,  80}, false,  true},
        {{-320,    0}, { -32,  -32,  32,  32}, false,  true},
        {{-288,  -96}, { -32,  -32,  32,  32}, false,  true},
        {{-240, -144}, { -16,  -16,  16,  16}, false, false},
        {{-144, -144}, { -48,  -16,  48,  16}, false, false},
        {{ 128, -144}, { -32,  -16,  32,  16}, false, false},
        -- south rooms
        {{-272, -192}, { -16,  -64,  16,  64}, false,  true},
        {{-112, -208}, { -16,  -48,  16,  48}, false,  true},
        {{-256, -288}, { -32,  -32,  32,  32}, false,  true},
        {{-192, -288}, { -32,  -32,  32,  32}, false, false},
        {{-128, -288}, { -32,  -32,  32,  32}, false,  true},
        {{ -64, -160}, { -32,  -32,  32,  32}, false,  true},
        {{ -48, -208}, { -16,  -16,  16,  16}, false,  true},
        -- walls
        {{ -96,  208}, { -32,  -16,  32,  16}, false,  true},
        {{ -48,  176}, { -16,  -16,  16,  16}, false,  true},
        {{ -96,  -64}, { -32,  -32,  32,  32}, false,  true},
        -- pillars
        {{ -80,  108}, { -16,  -20,  16,  20}, false,  true},
        {{ -48,   12}, { -16,  -20,  16,  20}, false,  true},
        {{-208,  -52}, { -16,  -20,  16,  20}, false,  true},
        {{ 208, -148}, { -16,  -20,  16,  20}, false, false},
    }) do
        local pos, bb, warp, mirror = table.unpack(t)
        local t = {pos = pos, collider = {type = Collider.AABB, bb = bb}}
        if warp then
            t.collider.flags = Collider.TRIGGER
        else
            t.collider.m = Math.INFINITY
            t.collider.flags = Collider.SOLID
        end
        local e = entity.load(nil, nil, t)
        table.insert(entities, e)
        if warp then table.insert(warps, e) end
        if mirror then
            t.pos[1] = -t.pos[1]
            table.insert(entities, entity.load(nil, nil, t))
        end
    end
    trigger = entity.load(nil, nil, {
        pos = {-80, -208},
        renderer = HIGHLIGHT_SPRITE.renderer,
        anim = HIGHLIGHT_SPRITE.anim,
        collider = {
            type = Collider.AABB,
            flags = Collider.SOLID | Collider.TRIGGER,
            bb = {-16, -16, 16, 16}, m = Math.INFINITY,
        },
    })
    table.insert(entities, trigger)
    local cur_map = map.name()
    if cur_map == "fe1" then
        player.move_all(0, 288, false)
    elseif cur_map == "fe2" then
        player.move_all(-192, -224, false)
    elseif cur_map == "fe3" then
        player.move_all(-272, 240, true)
    else
        player.move_all(0, -380, true)
    end
end

local function on_collision(e0, e1)
    if e0 == warps[1] or e1 == warps[1] then
        map.next("maps/fe1.lua")
    elseif e0 == warps[2] or e1 == warps[2] then
        map.next("maps/fe2.lua")
    elseif e0 == warps[3] or e1 == warps[3] then
        map.next("maps/fe3.lua")
    elseif e0 == warps[4] or e1 == warps[4] then
        map.next("maps/main.lua")
    elseif e0 == trigger or e1 == trigger then
        player.stop()
        FE:start()
    end
end

map.map {
    name = "fe0",
    file = "maps/fe0.lua",
    init = init,
--    reset = function() FE:reset() end,
    entities = entities,
    on_collision = on_collision,
    tiles = {
        "img/fire_emblem/21.png",
        32, 0, -16, 32, 32, 22, 23, {
             7, 22,   7, 23,   4, 26,   0, 30,   0, 30,   4, 26,   9, 24,   4, 26,   9, 24,   6, 23,   1, 22,  10, 31,   5, 23,   8, 24,   3, 24,   8, 23,   1, 31,  10, 31,   0, 30,   8, 22,   9, 22,   5, 23,
             2, 22,   3, 22,   4, 22,   4, 22,   4, 22,   0, 30,  10, 24,   4, 22,   4, 26,   0, 29,   8, 23,   1, 22,   5, 22,   9, 23,   0, 30,   0, 29,   6, 22,   3, 25,   4, 22,   0, 30,   5, 23,   0, 30,
            10, 23,   0, 22,   5, 28,   5, 28,   5, 28,   5, 28,   5, 28,   5, 28,   3, 23,   4, 26,   6, 23,   1, 22,  10, 25,   0, 30,   5, 28,  10, 28,   0, 27,   1, 27,  10, 28,   5, 27,   3, 23,   4, 23,
             7, 23,   5, 23,   8, 30,   2, 30,   2, 30,   2, 30,   2, 30,   3, 30,   6, 29,   9, 24,   6, 23,   8, 23,   9, 23,   5, 23,   8, 30,   1, 28,  10, 27,   1, 26,   1, 28,   7, 28,   6, 29,   6, 27,
             6, 27,   5, 23,   0, 23,  10, 27,   9, 28,   1, 23,  10, 27,   3, 31,   6, 29,   0, 30,   6, 23,   7, 25,   4, 23,   5, 23,   3, 31,  10, 27,   1, 26,   1, 26,   1, 26,   3, 31,   6, 29,   0, 30,
             0, 30,  10, 24,   0, 23,  10, 27,   1, 23,  10, 27,   1, 26,   3, 31,   6, 29,  10, 28,   2, 23,  10, 25,  10, 28,   3, 23,   3, 31,  10, 27,   1, 26,   0, 26,   0, 26,   3, 31,   6, 29,   4, 23,
             4, 26,   9, 24,   3, 31,   8, 28,   9, 28,   9, 28,   2, 27,   3, 31,   5, 27,   1, 28,   9, 31,  10, 25,   1, 28,   5, 27,   3, 31,   8, 28,   1, 26,   3, 26,   0, 26,   3, 31,   6, 29,   9, 24,
            10, 31,   6, 24,   5, 26,   6, 26,   7, 26,   8, 26,   9, 26,   7, 24,   9, 29,   9, 30,   9, 25,   7, 25,   1, 30,   9, 26,   7, 24,   6, 26,  10, 27,   2, 29,  10, 27,   3, 31,   6, 29,   8, 24,
            10, 25,   5, 28,   3, 31,  10, 27,   1, 26,   1, 26,   5, 25,   0, 24,   1, 24,   2, 24,   3, 24,   3, 24,   4, 24,   1, 24,   5, 24,   4, 25,  10, 27,   1, 26,   1, 26,   3, 31,   5, 27,   9, 31,
             7, 25,   6, 26,   9, 30,  10, 27,   3, 26,   0, 26,   2, 25,   5, 28,  10, 28,   0, 27,   8, 25,   8, 25,   1, 27,  10, 28,   5, 27,   0, 25,  10, 27,   3, 26,   0, 26,   1, 30,   6, 26,   9, 25,
             3, 25,   4, 25,   1, 26,   1, 26,   2, 29,  10, 27,   8, 30,   2, 30,   1, 28,  10, 27,   1, 26,   1, 26,   1, 26,   1, 28,   3, 27,   3, 30,  10, 27,   2, 29,  10, 27,   1, 26,   5, 25,   6, 25,
             5, 28,   0, 25,   1, 26,   1, 26,   1, 26,   1, 26,   3, 31,  10, 27,   1, 26,   3, 26,   0, 26,   0, 26,   3, 26,   0, 26,   1, 26,   1, 25,  10, 27,   1, 26,   1, 26,   1, 26,   2, 25,   5, 28,
             9, 29,   3, 30,   8, 28,   9, 28,   9, 28,   2, 27,   3, 31,  10, 27,   1, 26,   2, 29,  10, 27,   1, 26,   2, 29,  10, 27,   0, 26,   3, 31,   8, 28,   9, 28,   9, 28,   2, 27,   8, 30,   9, 29,
             4, 26,   5, 26,   9, 29,   6, 26,   7, 26,   8, 26,   9, 30,  10, 27,   1, 26,   0, 26,   1, 26,   0, 26,   1, 26,   0, 26,   1, 26,   1, 30,   6, 26,   7, 26,   8, 26,   9, 26,  10, 26,   6, 29,
             0, 30,   3, 31,  10, 27,   0, 26,   1, 26,   1, 26,   0, 26,   0, 26,   3, 26,   0, 26,   0, 26,   1, 26,   0, 26,   3, 26,   0, 26,   0, 26,   0, 26,   0, 26,   0, 26,   0, 26,   3, 31,   6, 29,
             2, 26,   3, 31,   8, 28,   9, 28,   2, 27,   9, 28,   2, 27,   0, 26,   2, 29,  10, 27,   0, 26,   1, 26,   1, 26,   2, 29,  10, 27,   9, 28,   2, 27,   2, 27,   9, 28,   2, 27,   3, 31,   6, 29,
             6, 27,   7, 27,   2, 28,   8, 27,   9, 27,   8, 27,   2, 28,  10, 27,   0, 26,   9, 28,   2, 27,   9, 28,   2, 27,   0, 26,   1, 26,   0, 28,   8, 27,   9, 27,   8, 27,   2, 28,   7, 27,   6, 29,
             5, 28,   6, 28,   2, 30,   1, 28,   4, 30,   1, 28,   7, 28,   8, 28,   9, 28,  10, 28,   0, 27,   1, 27,  10, 28,   8, 28,   2, 27,   8, 30,   1, 28,   4, 30,   1, 28,   3, 27,   4, 27,   5, 27,
             9, 29,   3, 31,   4, 30,   8, 31,   4, 29,   8, 31,   3, 31,  10, 29,   0, 28,   1, 28,   4, 30,   4, 29,   1, 28,   2, 28,   3, 28,   3, 31,   4, 30,   4, 29,   8, 31,   4, 29,   3, 31,   4, 28,
             1, 31,   3, 31,   4, 31,   7, 29,   8, 29,   4, 29,   3, 31,   8, 30,   2, 30,   9, 30,   4, 31,   8, 31,   1, 30,   2, 30,   3, 30,   3, 31,   4, 31,   8, 31,   4, 29,   8, 31,   3, 31,   6, 29,
             0, 29,   3, 31,   1, 29,   2, 29,   3, 29,   8, 31,   3, 31,   3, 31,   4, 30,   8, 31,   4, 29,   8, 31,   4, 29,   4, 29,   3, 31,   3, 31,   4, 30,   5, 29,   5, 29,   5, 29,   3, 31,   6, 29,
             0, 30,   1, 30,   2, 30,   2, 30,   3, 30,   4, 30,   3, 31,   3, 31,   4, 31,   5, 30,   6, 30,   6, 30,   7, 30,   8, 31,   3, 31,   3, 31,   4, 31,   8, 30,   2, 30,   2, 30,   9, 30,  10, 30,
             0, 31,   1, 31,   1, 31,   2, 31,   3, 31,   4, 31,   3, 31,   3, 31,   4, 31,   5, 31,   6, 31,   6, 31,   7, 31,   8, 31,   3, 31,   3, 31,   4, 31,   3, 31,   9, 31,   1, 31,   1, 31,  10, 31,
         },
     },
}
