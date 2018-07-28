local entity <const> = require "nngn.lib.entity"

local TYPE <const> = {
    EMPTY = 0,
    WET_SOIL = 1,
    WEED = 2,
    SOIL = 3,
    ROCK = 4,
    STUMP = 5,
    CORN_PLANTED = 6,
    CORN_WATERED = 7,
    CORN_PLANT0 = 8,
    CORN_PLANT1 = 9,
    CORN_PLANT2 = 10,
    CORN_RIPE = 11,
    REF = 12,
    N = 13,
}

local function time_cmp(x, y) return not (y.t < x.t) end

local function transform(self, obj, to)
    obj.type = to
    local e <const> = obj.entity
    nngn:renderers():remove(e:renderer())
    e:set_renderer(nil)
    local c <const> = e:collider()
    if c then
        nngn:colliders():remove(c)
        e:set_collider(nil)
    end
    self.load_entity(to, e)
    if to == TYPE.CORN_WATERED then
        obj.t = nngn:timing():now_ms() + 1000
        utils.insert_sorted(self.queue, obj, time_cmp)
    end
end

local objects <const> = {
    TYPE = TYPE,
}
objects.__index = objects

function objects:new(grid, ripe_crops)
    return setmetatable({
        grid = grid,
        ripe_crops = ripe_crops,
        v = {},
        queue = {},
    }, self)
end

function objects:reset()
    for _, x in pairs(self.v) do
        if x.entity then
            nngn:remove_entity(x.entity)
        end
    end
end

function objects:idx(x, y)
    return y * self.grid.size[1] + x
end

function objects:idx_abs(x, y)
    return self:idx(self.grid.pos(x, y))
end

function objects.load_entity(type, e)
    local t <const> = {
        renderer = {
            type = Renderer.SPRITE, tex = "img/harvest_moon.png",
            scale = {512//16, 512//16}, z_off = 8,
        },
    }
    if type == TYPE.WET_SOIL then
        t.renderer.coords = {7, 1}
        t.renderer.size = {16, 16}
    elseif type == TYPE.WEED then
        t.renderer.coords = {0, 2}
        t.renderer.size = {16, 16}
    elseif type == TYPE.SOIL then
        t.renderer.coords = {7, 0}
        t.renderer.size = {16, 16}
    elseif type == TYPE.ROCK then
        t.renderer.coords = {0, 3}
        t.renderer.size = {16, 16}
        t.collider = {
            type = Collider.AABB, flags = Collider.SOLID,
            bb = {-8, -8, 8, 8}, m = Math.INFINITY,
        }
    elseif type == TYPE.STUMP then
        t.renderer.coords = {1, 6, 3, 8}
        t.renderer.size = {32, 32}
        t.collider = {
            type = Collider.AABB, flags = Collider.SOLID,
            bb = {-12, -12, 12, 12}, m = Math.INFINITY,
        }
    elseif type == TYPE.CORN_PLANTED then
        t.renderer.coords = {7, 2}
        t.renderer.size = {16, 16}
    elseif type == TYPE.CORN_WATERED then
        t.renderer.coords = {7, 3}
        t.renderer.size = {16, 16}
    elseif type == TYPE.CORN_PLANT0 then
        t.renderer.coords = {9, 7}
        t.renderer.size = {16, 16}
        t.collider = {
            type = Collider.AABB, flags = Collider.SOLID,
            bb = {-8, -8, 8, 8}, m = Math.INFINITY,
        }
    elseif type == TYPE.CORN_PLANT1 then
        t.renderer.coords = {9, 6}
        t.renderer.size = {16, 16}
        t.collider = {
            type = Collider.AABB, flags = Collider.SOLID,
            bb = {-8, -8, 8, 8}, m = Math.INFINITY,
        }
    elseif type == TYPE.CORN_PLANT2 then
        t.renderer.coords = {9, 5}
        t.renderer.size = {16, 16}
        t.collider = {
            type = Collider.AABB, flags = Collider.SOLID,
            bb = {-8, -8, 8, 8}, m = Math.INFINITY,
        }
    elseif type == TYPE.CORN_RIPE then
        t.renderer.coords = {9, 4}
        t.renderer.size = {16, 16}
        t.collider = {
            type = Collider.AABB, flags = Collider.SOLID | Collider.TRIGGER,
            bb = {-8, -8, 8, 8}, m = Math.INFINITY,
        }
    else
        error("invalid type")
    end
    return entity.load(e, nil, t)
end

function objects:create(type, x, y)
    local v <const> = self.v
    local i <const> = self:idx(x, y)
    if v[i] then
        return
    end
    local e <const> = self.load_entity(type)
    local t <const> = {type = type, entity = e}
    v[i] = t
    local x, y = self.grid.abs_pos(x, y)
    if type == TYPE.STUMP then
        local ref <const> = {type = TYPE.REF, i = i}
        local i0 <const> = i + 1
        local i1 <const> = i + self.grid.size[1]
        local i2 <const> = i1 + 1
        t.refs = {i0, i1, i2}
        v[i0] = ref
        v[i1] = ref
        v[i2] = ref
        x = x + self.grid.SIZE / 2
        y = y + self.grid.SIZE / 2
    end
    e:set_pos(x, y, 0)
end

function objects:destroy(type, x, y)
    local v <const> = self.v
    local i = self:idx(x, y)
    local obj = v[i]
    if not obj then
        return
    end
    if obj.type == TYPE.REF then
        i = obj.i
        obj = v[i]
    end
    if type ~= obj.type then
        return
    end
    nngn:remove_entity(obj.entity)
    v[i] = nil
    local refs <const> = obj.refs
    if refs then
        for _, i in ipairs(refs) do
            v[i] = nil
        end
    end
end

function objects:transform_obj(to, obj)
    transform(self, obj, to)
end

function objects:transform_pos(map, x, y)
    local obj <const> = self.v[self:idx(x, y)]
    if not obj then
        return
    end
    local to <const> = map[obj.type]
    if not to then
        return
    end
    self:transform_obj(to, obj)
end

function objects:update()
    local now <const> = nngn:timing():now_ms()
    local q <const> = self.queue
    for i, obj in pairs(q) do
        local t, type = obj.t, obj.type
        if now < t then
            break
        end
        if TYPE.CORN_WATERED <= type and type < TYPE.CORN_RIPE then
            type = type + 1
            transform(self, obj, type)
            table.remove(q, i)
            if type == TYPE.CORN_RIPE then
                obj.t = nil
                self.ripe_crops[deref(obj.entity)] = obj
            else
                obj.t = t + 1000
                utils.insert_sorted(q, obj, time_cmp)
            end
        end
    end
end

return objects
