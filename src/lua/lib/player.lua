local entity = require "nngn.lib.entity"
local utils = require "nngn.lib.utils"

local cur
local list = {}
local preset

local function get_entity(p)
    p = p or list[cur]
    if p then
        return p.entity
    end
end

local function load(e)
    if not preset then
        error("no player presets loaded")
    end
    e = e or get_entity()
    entity.load(e, preset, {})
end

local function add(e)
    if not e then
        e = assert(nngn.entities:add())
        load(e, true)
    end
    local ret <const> = {
        entity = e,
    }
    table.insert(list, ret)
    if #list == 1 then
        cur = #list
    end
    return ret
end

local function remove(p)
    p = p or list[cur]
    if not p then
        return
    end
    nngn:remove_entity(p.entity)
    table.remove(list, utils.find(list, p))
    cur = math.min(cur, #list)
end

local function next(inc)
    local p = list[cur]
    if not p then
        return
    end
    inc = inc or 1
    cur = utils.shift(cur, #list, inc, 1)
end

return {
    set = function(p) preset = p end,
    n = function() return #list end,
    cur = function() return list[cur] end,
    idx = function() return cur - 1 end,
    get = function(i) return list[i + 1] end,
    entity = get_entity,
    add = add,
    remove = remove,
    load = load,
    next = next,
}
