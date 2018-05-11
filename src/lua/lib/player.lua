local entity = require "nngn.lib.entity"
local utils = require "nngn.lib.utils"

local PLAYER

local function set(t) PLAYER = t end

local function load(e)
    if PLAYER == nil then error("no player loaded") end
    entity.load(e, PLAYER, {})
end

local function add()
    local e = nngn.entities:add()
    local p = nngn.players:add(e)
    load(e, true)
    return p
end

local function remove(p)
    nngn:remove_entity(p:entity())
    nngn.players:remove(p)
end

local function next(inc)
    inc = inc or 1
    local n = nngn.players:n()
    if n == 0 then return end
    nngn.players:set_idx(utils.shift(nngn.players:idx(), n, inc))
end

return {
    set = set,
    load = load,
    add = add,
    remove = remove,
    next = next,
}
