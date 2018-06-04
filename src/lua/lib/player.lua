local camera = require "nngn.lib.camera"
local entity = require "nngn.lib.entity"
local utils = require "nngn.lib.utils"

local FACE <const> = {
    LEFT = 00, RIGHT = 01, DOWN = 02, UP = 03, N = 4,
}

local MAX_VEL = camera.MAX_VEL
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
        face = FACE.DOWN,
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

local function stop(p)
    p = p or list[cur]
    if not p then
        return
    end
    local e <const> = p.entity
    e:set_vel(0, 0, 0)
end

local function next(inc)
    local p = list[cur]
    if not p then
        return
    end
    inc = inc or 1
    stop(p)
    cur = utils.shift(cur, #list, inc, 1)
end

local function on_face_change() end

local function face_for_dir(hor, ver)
    if hor ~= 0 then
        if hor < 0 then return FACE.LEFT
        else return FACE.RIGHT end
    else
        if ver < 0 then return FACE.DOWN
        else return FACE.UP end
    end
end

local function move(key, press, _, keys)
    local p = list[cur]
    if not p then
        return
    end
    keys = keys or nngn.input:get_keys{
        string.byte("A"), string.byte("D"),
        string.byte("S"), string.byte("W"),
    }
    local dir = {keys[2] - keys[1], keys[4] - keys[3]}
    local l = math.abs(dir[1]) + math.abs(dir[2])
    local v, face, anim
    if l == 0 then
        v = 0
        face = p.face % FACE.N
    elseif l == 1 then
        v = MAX_VEL
        face = face_for_dir(table.unpack(dir))
    else
        v = MAX_VEL / math.sqrt(2)
    end
    if face then
        p.face = face
        on_face_change(p, face)
    end
    local e <const> = p.entity
    e:set_vel(dir[1] * v, dir[2] * v, 0)
end

return {
    FACE = FACE,
    MAX_VEL = MAX_VEL,
    set = function(p) preset = p end,
    n = function() return #list end,
    cur = function() return list[cur] end,
    idx = function() return cur - 1 end,
    get = function(i) return list[i + 1] end,
    entity = get_entity,
    add = add,
    remove = remove,
    load = load,
    stop = stop,
    next = next,
    move = move,
}
