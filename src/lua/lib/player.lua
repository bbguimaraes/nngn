local camera = require "nngn.lib.camera"
local entity = require "nngn.lib.entity"
local utils = require "nngn.lib.utils"

local MAX_VEL = camera.MAX_VEL
local PLAYERS = {}
local LAST_LOADED = 0
local DATA = {}

local function set(t) PLAYERS = t end

local function data(p, f)
    if not p then p = nngn.players:cur() end
    if not p then return end
    local d = DATA[deref(p)]
    if not d then d = {} DATA[deref(p)] = d end
    if not f then return d end
    if type(f) ~= "table" then f = {f} end
    for _, x in ipairs(f) do
        local t = d[x]
        if not t then t = {} d[x] = t end
        d = t
    end
    return d
end

local function set_data(p, t) DATA[deref(p)] = t end

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
    local d = data(p)
    if d then
        if d.fairy then nngn:remove_entity(d.fairy) end
        if d.fairy_light then nngn:remove_entity(d.fairy_light) end
        if d.light then nngn:remove_entity(d.light) end
        if d.flashlight then nngn:remove_entity(d.flashlight) end
        set_data(p, nil)
    end
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

local function light(p, show)
    p = p or nngn.players:cur()
    if not p then return end
    local d = data(p)
    if show == nil then show = not d.light end
    if show then
        local pos = {p:face_vec(8)}
        pos[2] = pos[2] + p:entity():renderer():z_off()
        if pos[1] ~= 0 then pos[2] = pos[2] - 4 end
        pos[3] = 24
        if d.light then
            nngn.entities:set_pos(d.light, table.unpack(pos))
        else
            d.light = entity.load(nil, nil, {
                pos = pos, parent = p:entity(),
                light = {
                    type = Light.POINT,
                    color = {1, .8, .5, 1}, att = 512}})
        end
    elseif d.light then
        nngn:remove_entity(d.light)
        d.light = nil
    end
end

local function flashlight(p, show)
    p = p or nngn.players:cur()
    if not p then return end
    local d = data(p)
    if show == nil then show = not d.flashlight end
    if show then
        local dir = {p:face_vec(1)}
        dir[3] = 0
        if d.flashlight then
            d.flashlight:light():set_dir(table.unpack(dir))
        else
            d.flashlight = entity.load(nil, nil, {
                pos = {0, p:entity():renderer():z_off(), 12},
                parent = p:entity(),
                light = {
                    type = Light.POINT, dir = dir, color = {1, 1, 1, 1},
                    att = 512, cutoff = math.cos(math.rad(22.5))}})
        end
    elseif d.flashlight then
        nngn:remove_entity(d.flashlight)
        d.flashlight = nil
    end
end

local function on_face_change(p)
    local d = data(p)
    if d.light then light(p, true) end
    if d.flashlight then flashlight(p, true) end
end

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

local function move_all(x, y, abs)
    for i = 0, nngn.players:n() - 1 do
        local e = nngn.players:get(i):entity()
        local px, py, pz = e:pos()
        if abs then nngn.entities:set_pos(e, x, y - e:renderer():z_off(), pz)
        else nngn.entities:set_pos(e, px + x, py + y, pz) end
    end
    if not camera.following() then
        local _, _, z = nngn.camera:pos()
        nngn.camera:set_pos(x, y, z)
    end
end

local function fairy(p, show)
    p = p or nngn.players:cur()
    if not p then return end
    local d = data(p)
    if d.fairy then
        if show ~= nil and show then return end
        nngn:remove_entity(d.fairy)
        nngn:remove_entity(d.fairy_light)
        d.fairy = nil
        d.fairy_light = nil
    else
        if show ~= nil and not show then return end
        local t = dofile("src/lson/fairy2.lua")
        t.pos = {-8, 16, 0}
        t.renderer.type = Renderer.TRANSLUCENT
        t.renderer.z_off = -40
        t.parent = p:entity()
        d.fairy = entity.load(nil, nil, t)
        d.fairy_light = entity.load(nil, nil, {
            pos = {-8, p:entity():renderer():z_off() - 12, 38},
            parent = t.parent,
            light = {type = Light.POINT, color = {.5, .5, 1, 1}, att = 512},
            anim = {light = {
                rate_ms = 100,
                f = {type = AnimationFunction.RANDOM_F, min = .85, max = 1}}}})
    end
end

local function fire(show)
    local p = nngn.players:cur()
    if not p then return end
    local d = data(p, "fire")
    if show then
        if d.entity then d.remove() end
        local t = dofile("src/lson/fire.lua")
        local pos = {p:face_vec(16)}
        pos[2] = pos[2] + p:entity():renderer():z_off() + 8
        t.pos = pos
        t.renderer.type = Renderer.TRANSLUCENT
        t.parent = p:entity()
        d.vel = {p:face_vec(MAX_VEL * 8)}
        d.vel[3] = 0
        d.entity = entity.load(nil, nil, t)
        pos[2] = pos[2]  - 12
        pos[3] = 8
        d.light = entity.load(nil, nil, {
            parent = p:entity(), pos = pos,
            light = {type = Light.POINT, color = {1, .2, 0, 1}, att = 2048},
            anim = {light = {
                rate_ms = 50,
                f = {type = AnimationFunction.RANDOM_F, min = .8, max = 1}}}})
        d.remove = function()
            nngn:remove_entity(d.entity)
            nngn:remove_entity(d.light)
            d.entity, d.light = nil, nil
        end
    else
        local p0 = {p:entity():pos()}
        local p1 = {d.entity:pos()}
        d.entity:set_parent(nil)
        nngn.entities:set_pos(d.entity, p0[1] + p1[1], p0[2] + p1[2], 0)
        d.entity:set_vel(table.unpack(d.vel))
        entity.load(d.entity, nil, {
            collider = {
                type = Collider.AABB,
                bb = {-4, -8, 4, -4}, flags = Collider.TRIGGER}})
        d.light:set_parent(d.entity)
        nngn.entities:set_pos(d.light, 0, -16, 8)
    end
end

return {
    MAX_VEL = MAX_VEL,
    set = set,
    data = data,
    load = load,
    add = add,
    remove = remove,
    stop = stop,
    next = next,
    move = move,
    move_all = move_all,
    fairy = fairy,
    light = light,
    flashlight = flashlight,
    fire = fire,
}
