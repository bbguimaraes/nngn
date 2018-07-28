local entity <const> = require "nngn.lib.entity"
local input <const> = require "nngn.lib.input"
local nngn_math <const> = require "nngn.lib.math"
local texture <const> = require "nngn.lib.texture"
local ui <const> = require "nngn.lib.ui"

local TYPE <const> = {
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
    CORN = 10,
    N = 11,
}

local function reset(self)
    nngn:remove_entities(self.entities)
    self.entities = nil
end

local tools <const> = {}
tools.__index = tools

function tools:new(objects, chicken)
    local ret <const> = setmetatable({
        wheel = ui.wheel:new(TYPE.N, 32),
        objects = objects,
        chicken = chicken,
    }, self)
    ret.actions = ret:init_actions()
    ret.input = ret:init_input()
    return ret
end

function tools:init_actions()
    local o <const> = self.objects
    local T <const>, OT <const> = TYPE, o.TYPE
    local t_water <const> = {
        [OT.SOIL] = OT.WET_SOIL,
        [OT.CORN_PLANTED] = OT.CORN_WATERED,
    }
    local t_corn <const>  = {
        [OT.SOIL] = OT.CORN_PLANTED,
        [OT.WET_SOIL] = OT.CORN_WATERED,
    }
    return {
        [T.WATERING_CAN]  = function(x, y) o:transform_pos(t_water, x, y) end,
        [T.SICKLE]        = function(x, y) o:destroy(OT.WEED, x, y) end,
        [T.HOE]           = function(x, y) o:create(OT.SOIL, x, y) end,
        [T.HAMMER]        = function(x, y) o:destroy(OT.ROCK, x, y) end,
        [T.AXE]           = function(x, y) o:destroy(OT.STUMP, x, y) end,
        [T.CORN]          = function(x, y) o:transform_pos(t_corn, x, y) end,
        [T.SPRINKLER]     = function(x, y) self.chicken(x, y, false) end,
        [T.GOLDEN_SICKLE] = function(x, y) o:create(OT.WEED, x, y) end,
        [T.GOLDEN_HOE]    = function(x, y) self.chicken(x, y, true) end,
        [T.GOLDEN_HAMMER] = function(x, y) o:create(OT.ROCK, x, y) end,
        [T.GOLDEN_AXE]    = function(x, y) o:create(OT.STUMP, x, y) end,
    }
end

function tools:init_input()
    local ret <const> = BindingGroup:new()
    ret:set_next(input.input)
    ret:add(string.byte("E"), Input.SEL_PRESS, function(_, _, mods)
        if mods & Input.MOD_SHIFT == 0 then
            self:cycle(1)
        else
            self:cycle(-1)
        end
    end)
    ret:add(string.byte("F"), Input.SEL_PRESS, function()
        self:dismiss()
    end)
    return ret
end

function tools:active()
    return self.entities ~= nil
end

function tools:reset()
    if self:active() then
        reset(self)
    end
end

function tools:load(e)
    local entities <const> = {}
    local tex <close> = texture.load("img/harvest_moon.png")
    local t <const> = {
        parent = e,
        renderer = {
            type = Renderer.SPRITE, tex = tex.tex,
            size = {16, 16}, scale = {512//16, 512//16}, z_off = -8,
        },
    }
    for y = 0, 1 do
        for x = 0, 4 do
            t.renderer.coords = {14 + x, 30 + y}
            table.insert(entities, entity.load(nil, nil, t))
        end
    end
    t.renderer.coords = {8, 3}
    table.insert(entities, entity.load(nil, nil, t))
    self.entities = entities
end

function tools:update(dt)
    if not self:active() then
        return
    end
    local t <const> = self.entities
    local n <const> = #t
    self.wheel:update(dt, function(i, x, y)
        i = (n - i) % n
        t[i + 1]:set_pos(x, y, 0)
    end)
end

function tools:activate(e)
    self:load(e)
    local i <const> = nngn:input()
    self.prev_input = i:binding_group()
    i:set_binding_group(self.input)
end

function tools:dismiss()
    reset(self)
    nngn:input():set_binding_group(self.prev_input)
    self.wheel:dismiss()
end

function tools:cycle(dir)
    local n <const> = #self.entities
    self.wheel:cycle(dir)
end

function tools:action(x, y)
    self.actions[self.wheel.pos](x, y)
end

return tools
