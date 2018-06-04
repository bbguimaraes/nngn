local camera = require "nngn.lib.camera"
local entity = require "nngn.lib.entity"
local utils = require "nngn.lib.utils"

local MAX_VEL = camera.MAX_VEL
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

local function stop(p)
    local e = p:entity()
    e:set_vel(0, 0, 0)
end

local function next(inc)
    inc = inc or 1
    local p = nngn.players:cur()
    if not p then return end
    stop(p)
    nngn.players:set_idx(
        utils.shift(nngn.players:idx(), nngn.players:n(), inc))
end

local function on_face_change() end

local function face_for_dir(hor, ver)
    if hor ~= 0 then
        if hor < 0 then return Player.FLEFT
        else return Player.FRIGHT end
    else
        if ver < 0 then return Player.FDOWN
        else return Player.FUP end
    end
end

local function move(_, _, _, keys)
    local p = nngn.players:cur()
    if not p then return end
    keys = keys or nngn.input:get_keys{
        string.byte("A"), string.byte("D"),
        string.byte("S"), string.byte("W")}
    local dir = {keys[2] - keys[1], keys[4] - keys[3]}
    local l = math.abs(dir[1]) + math.abs(dir[2])
    local v, face
    if l == 0 then
        v = 0
        face = p:face() % Player.N_FACES
    elseif l == 1 then
        v = MAX_VEL
        face = face_for_dir(table.unpack(dir))
    else
        v = MAX_VEL / math.sqrt(2)
    end
    if face then p:set_face(face) on_face_change(p, face) end
    p:entity():set_vel(dir[1] * v, dir[2] * v, 0)
end

return {
    MAX_VEL = MAX_VEL,
    set = set,
    load = load,
    add = add,
    remove = remove,
    stop = stop,
    next = next,
    move = move,
}
