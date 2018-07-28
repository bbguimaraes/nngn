local state = require "nngn.lib.state"
local texture = require "nngn.lib.texture"

local MAP = {}

local function name() return MAP.name end
local function data() return MAP.data end

local function cancel()
    if not MAP.heartbeat then return end
    nngn.schedule:cancel(MAP.heartbeat)
    MAP.heartbeat = nil
end

local function map(t)
    if MAP.state then state.restore(MAP.state) end
    if MAP.reset then MAP.reset() end
    if MAP.entities then nngn:remove_entity_v(MAP.entities) end
    cancel()
    if t.state then state.save(t.state) end
    if t.tiles then
        local tex <close> = texture.load(t.tiles[1])
        t.tiles[1] = tex.tex
        nngn.map:load(table.unpack(t.tiles))
    end
    if t.init then t.init() end
    if t.heartbeat then
        t.heartbeat = nngn.schedule:next(Schedule.HEARTBEAT, t.heartbeat)
    end
    MAP = t
end

local function next(f)
    cancel()
    nngn.schedule:next(0, function() dofile(f) end)
end

local function key_callback(...)
    if MAP.key_callback then return MAP.key_callback(...) end
end

local function on_collision(...)
    if MAP.on_collision then MAP.on_collision(...) end
end

return {
    name = name,
    data = data,
    map = map,
    next = next,
    key_callback = key_callback,
    on_collision = on_collision,
}
