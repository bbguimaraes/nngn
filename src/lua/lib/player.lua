local camera = require "nngn.lib.camera"
local entity = require "nngn.lib.entity"
local utils = require "nngn.lib.utils"

local MAX_VEL = camera.MAX_VEL
local PLAYERS = {}
local LAST_LOADED = 0

local function set(t) PLAYERS = t end

local function load(e, inc)
    if #PLAYERS == 0 then error("no player loaded") end
    LAST_LOADED = utils.shift(LAST_LOADED, #PLAYERS, inc, 1)
    local t = dofile(PLAYERS[LAST_LOADED])
    if t.collider then t.collider.flags = Collider.SOLID end
    entity.load(e, nil, t)
    local anim = e:animation()
    if anim then
        local sprite = anim:sprite()
        if sprite and sprite:track_count() > 1
        then sprite:set_track(Player.FDOWN) end
    end
end

local function add()
    local e = nngn.entities:add()
    local p = nngn.players:add(e)
    load(e, true)
    return p
end

local function remove(p)
    local e <const> = p:entity()
    nngn.players:remove(p)
    if e == camera.following() then
        p = nngn.players:cur()
        camera.set_follow(p and p:entity())
    end
    nngn:remove_entity(e)
end

local function stop(p)
    local e = p:entity()
    e:set_vel(0, 0, 0)
    p:set_running(false)
    local a = e:animation()
    if a then
        local sprite = a:sprite()
        if sprite then
            sprite:set_track(sprite:cur_track() % Player.N_FACES)
         end
    end
end

local function next(inc)
    inc = inc or 1
    local p = nngn.players:cur()
    if not p then return end
    stop(p)
    local p = nngn.players:set_idx(
        utils.shift(nngn.players:idx(), nngn.players:n(), inc))
        local e = p:entity()
    if camera.following() then camera.set_follow(e) end
    nngn.renderers:add_selection(e:renderer())
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

local function set_anim_track(e, t)
    local a = e:animation()
    if not a then return end
    local s = a:sprite()
    if s and s:cur_track() ~= t and t < s:track_count() then
        s:set_track(t)
    end
end

local function move(key, press, _, keys)
    local p = nngn.players:cur()
    if not p then return end
    keys = keys or nngn.input:get_keys{
        string.byte("A"), string.byte("D"),
        string.byte("S"), string.byte("W")}
    local dir = {keys[2] - keys[1], keys[4] - keys[3]}
    local l = math.abs(dir[1]) + math.abs(dir[2])
    local running = p:running()
    local v, face, anim
    if l == 0 then
        v = 0
        face = p:face() % Player.N_FACES
        anim = face
        p:set_running(false)
    elseif l == 1 then
        v = MAX_VEL
        face = face_for_dir(table.unpack(dir))
        if press and utils.check_double_tap(nngn.timing:now_ms(), key) then
            running = true
            p:set_running(true)
        end
        if running then anim = face + Player.RUN
        else anim = face + Player.WALK end
    else
        v = MAX_VEL / math.sqrt(2)
    end
    if face then p:set_face(face) on_face_change(p, face) end
    local e = p:entity()
    if anim then set_anim_track(e, anim) end
    if running then v = v * 3 end
    e:set_vel(dir[1] * v, dir[2] * v, 0)
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
