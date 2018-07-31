local camera = require "nngn.lib.camera"
local entity = require "nngn.lib.entity"
local utils = require "nngn.lib.utils"

local FACE <const> = {
    LEFT = 00, RIGHT = 01, DOWN = 02, UP = 03, N = 4,
}
local ANIMATION <const> = {
    FACE   = 0 * FACE.N,
    FLEFT  = 0 * FACE.N + FACE.LEFT,
    FRIGHT = 0 * FACE.N + FACE.RIGHT,
    FDOWN  = 0 * FACE.N + FACE.DOWN,
    FUP    = 0 * FACE.N + FACE.UP,
    WALK   = 1 * FACE.N,
    WLEFT  = 1 * FACE.N + FACE.LEFT,
    WRIGHT = 1 * FACE.N + FACE.RIGHT,
    WDOWN  = 1 * FACE.N + FACE.DOWN,
    WUP    = 1 * FACE.N + FACE.UP,
    RUN    = 2 * FACE.N,
    RLEFT  = 2 * FACE.N + FACE.LEFT,
    RRIGHT = 2 * FACE.N + FACE.RIGHT,
    RDOWN  = 2 * FACE.N + FACE.DOWN,
    RUP    = 2 * FACE.N + FACE.UP,
    N      = 3 * FACE.N,
}

local MAX_VEL = camera.MAX_VEL
local cur
local list = {}
local presets
local last_loaded = 0

local function get_entity(p)
    p = p or list[cur]
    if p then
        return p.entity
    end
end

local function load(e, inc)
    if not presets then
        error("no player presets loaded")
    end
    e = e or get_entity()
    last_loaded = utils.shift(last_loaded, #presets, inc, 1)
    local t = dofile(presets[last_loaded])
    if t.collider then t.collider.flags = Collider.SOLID end
    entity.load(e, nil, t)
    local anim = e:animation()
    if anim then
        local sprite = anim:sprite()
        if sprite and sprite:track_count() > 1
        then sprite:set_track(ANIMATION.FDOWN) end
    end
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
        camera.set_follow(e)
    end
    return ret
end

local function remove(p)
    p = p or list[cur]
    if not p then
        return
    end
    local e <const> = p.entity
    table.remove(list, utils.find(list, p))
    if e == camera.following() then
        p = list[cur]
        camera.set_follow(p and p.entity)
    end
    nngn:remove_entity(e)
    cur = math.min(cur, #list)
end

local function stop(p)
    p = p or list[cur]
    if not p then
        return
    end
    local e <const> = p.entity
    e:set_vel(0, 0, 0)
    p.running = false
    local a <const> = e:animation()
    if a then
        local sprite = a:sprite()
        if sprite then
            sprite:set_track(sprite:cur_track() % FACE.N)
         end
    end
end

local function next(inc)
    local p = list[cur]
    if not p then
        return
    end
    inc = inc or 1
    stop(p)
    cur = utils.shift(cur, #list, inc, 1)
    local e <const> = list[cur].entity
    if camera.following() then
        camera.set_follow(e)
    end
    nngn.renderers:add_selection(e:renderer())
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

local function set_anim_track(e, t)
    local a = e:animation()
    if not a then
        return
    end
    local s = a:sprite()
    if s and s:cur_track() ~= t and t < s:track_count() then
        s:set_track(t)
    end
end

local function move(key, press, _, keys)
    local p = list[cur]
    if not p then
        return
    end
    keys = keys or nngn.input:get_keys{
        string.byte("A"), string.byte("D"),
        string.byte("S"), string.byte("W")}
    local dir = {keys[2] - keys[1], keys[4] - keys[3]}
    local l = math.abs(dir[1]) + math.abs(dir[2])
    local v, face, anim
    if l == 0 then
        v = 0
        face = p.face % FACE.N
        anim = face
        p.running = false
    elseif l == 1 then
        v = MAX_VEL
        face = face_for_dir(table.unpack(dir))
        if press and utils.check_double_tap(nngn.timing:now_ms(), key) then
            p.running = true
        end
        if p.running then
            anim = face + ANIMATION.RUN
        else
            anim = face + ANIMATION.WALK
        end
    else
        v = MAX_VEL / math.sqrt(2)
    end
    if face then
        p.face = face
        on_face_change(p, face)
    end
    local e <const> = p.entity
    if anim then
        set_anim_track(e, anim)
    end
    if p.running then
        v = v * 3
    end
    e:set_vel(dir[1] * v, dir[2] * v, 0)
end

return {
    FACE = FACE,
    ANIMATION = ANIMATION,
    MAX_VEL = MAX_VEL,
    set = function(t) presets = t end,
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
