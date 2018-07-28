local state = require "nngn.lib.state"
local texture = require "nngn.lib.texture"

local MAP = {}

local function name() return MAP.name end
local function file() return MAP.file end
local function data() return MAP.data end

local function call(msg, name, f, ...)
    local ok <const>, err <const> = xpcall(f, debug.traceback, ...)
    if not ok then
        io.stderr:write(string.format(msg, name, err))
    end
    return ok
end

local function cancel()
    if not MAP.heartbeat then return end
    nngn.schedule:cancel(MAP.heartbeat)
    MAP.heartbeat = nil
end

local function init(t)
    if t.state then state.save(t.state) end
    if t.tiles then
        local tex <close> = texture.load(t.tiles[1])
        t.tiles[1] = tex.tex
        nngn.map:load(table.unpack(t.tiles))
    end
    if t.init then t.init() end
    if t.heartbeat then
        t.heartbeat = nngn.schedule:next(
            Schedule.IGNORE_FAILURES | Schedule.HEARTBEAT,
            t.heartbeat)
    end
end

local function reset(t)
    if MAP.state then
        state.restore(MAP.state)
        MAP.state = nil
    end
    if MAP.reset then
        MAP.reset()
        MAP.reset = nil
    end
    if MAP.entities then
        nngn:remove_entity_v(MAP.entities)
        MAP.entities = nil
    end
    cancel()
end

local function map(t)
    reset(MAP)
    if call("map.map: failed to initialize map %q: %s\n", t.name, init, t) then
        MAP = t
    end
end

local function next(f)
    cancel()
    nngn.schedule:next(Schedule.IGNORE_FAILURES, function()
        call("map.next: failed to read map file %q: %s\n", f, dofile, f)
    end)
end

local function key_callback(...)
    if MAP.key_callback then return MAP.key_callback(...) end
end

local function on_collision(...)
    if MAP.on_collision then MAP.on_collision(...) end
end

return {
    name = name,
    file = file,
    data = data,
    map = map,
    next = next,
    key_callback = key_callback,
    on_collision = on_collision,
}
