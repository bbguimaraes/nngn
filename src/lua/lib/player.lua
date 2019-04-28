local camera = require "nngn.lib.camera"
local entity = require "nngn.lib.entity"
local menu <const> = require("nngn.lib.menu")
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

local MAX_VEL = camera.MAX_VEL / 4
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
        e = assert(nngn:entities():add())
        load(e, true)
    end
    local ret <const> = {
        entity = e,
        face = FACE.DOWN,
        data = {
            menu = menu.new_player(),
        },
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
    local d <const> = p.data
    if d.fairy then nngn:remove_entity(d.fairy) end
    if d.fairy_light then nngn:remove_entity(d.fairy_light) end
    if d.light then nngn:remove_entity(d.light) end
    if d.flashlight then nngn:remove_entity(d.flashlight) end
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
    nngn:renderers():add_selection(e:renderer())
end

local function face_vec(p, l)
    if p.face & 2 == 0 then
        return {l * ((p.face << 1) - 1), 0}
    else
        return {0, l * (((p.face & 1) << 1) - 1)}
    end
end

local function light(p, show)
    p = p or list[cur]
    if not p then
        return
    end
    local d <const> = p.data
    if show == nil then
        show = not d.light
    end
    if show then
        local pos <const> = face_vec(p, 8)
        pos[2] = pos[2] + p.entity:renderer():z_off()
        if pos[1] ~= 0 then
            pos[2] = pos[2] - 4
        end
        pos[3] = 24
        if d.light then
            d.light:set_pos(table.unpack(pos))
        else
            d.light = entity.load(nil, nil, {
                pos = pos, parent = p.entity,
                light = {
                    type = Light.POINT,
                    color = {1, .8, .5, 1}, att = 512},
                })
        end
    elseif d.light then
        nngn:remove_entity(d.light)
        d.light = nil
    end
end

local function flashlight(p, show)
    p = p or list[cur]
    if not p then
        return
    end
    local d <const> = p.data
    if show == nil then
        show = not d.flashlight
    end
    if show then
        local dir <const> = face_vec(p, 1)
        dir[3] = 0
        if d.flashlight then
            d.flashlight:light():set_dir(table.unpack(dir))
        else
            d.flashlight = entity.load(nil, nil, {
                pos = {0, p.entity:renderer():z_off(), 12},
                parent = p.entity,
                light = {
                    type = Light.POINT, dir = dir, color = {1, 1, 1, 1},
                    att = 512, cutoff = math.cos(math.rad(22.5))},
                })
        end
    elseif d.flashlight then
        nngn:remove_entity(d.flashlight)
        d.flashlight = nil
    end
end

local function on_face_change(p)
    p = p or list[cur]
    if not p then
        return
    end
    local d <const> = p.data
    if d.light then
        light(p, true)
    end
    if d.flashlight then
        flashlight(p, true)
    end
end

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
    keys = keys or nngn:input():get_keys{
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
        if press and utils.check_double_tap(nngn:timing():now_ms(), key) then
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

local function move_all(x, y, abs)
    for _, p in ipairs(list) do
        local e <const> = p.entity
        local px, py, pz = e:pos()
        if abs then
            e:set_pos(x, y - e:renderer():z_off(), pz)
        else
            e:set_pos(px + x, py + y, pz)
        end
    end
    if not camera.following() then
        local _, _, z = nngn:camera():pos()
        nngn:camera():set_pos(x, y, z)
    end
end

local function player_menu(p, mods)
    p = p or list[cur]
    if p then
        menu.menu(p, mods)
    end
end

local function action(p, press)
    p = p or list[cur]
    if not p then
        return
    end
    local m <const> = p.data.menu
    if menu.displayed(m) then
        menu.hide(m)
    else
        m.actions[m.idx](p, press)
    end
end

local function fairy(p, show)
    p = p or list[cur]
    if not p then
        return
    end
    local d <const> = p.data
    if d.fairy then
        if show ~= nil and show then
            return
        end
        nngn:remove_entity(d.fairy)
        nngn:remove_entity(d.fairy_light)
        d.fairy = nil
        d.fairy_light = nil
    else
        if show ~= nil and not show then
            return
        end
        local e <const> = p.entity
        local t = dofile("src/lson/zelda/fairy2.lua")
        t.pos = {-8, 16, 0}
        t.renderer.type = Renderer.TRANSLUCENT
        t.renderer.z_off = -40
        t.parent = e
        d.fairy = entity.load(nil, nil, t)
        d.fairy_light = entity.load(nil, nil, {
            pos = {-8, e:renderer():z_off() - 12, 38},
            parent = t.parent,
            light = {type = Light.POINT, color = {.5, .5, 1, 1}, att = 512},
            anim = {
                light = {
                    rate_ms = 100,
                    f = {
                        type = AnimationFunction.RANDOM_F,
                        min = .85, max = 1,
                    },
                },
            },
        })
    end
end

local function fire(p, show)
    p = p or list[cur]
    if not p then
        return
    end
    local d = p.data
    if d.fire then
        d = d.fire
    else
        d = {}
        p.data.fire = d
    end
    if show then
        if d.entity then
            d.remove()
        end
        local t <const> = dofile("src/lson/zelda/fire.lua")
        local pos <const> = face_vec(p, 16)
        pos[2] = pos[2] + p.entity:renderer():z_off() + 8
        t.pos = pos
        t.renderer.type = Renderer.TRANSLUCENT
        t.parent = p.entity
        d.vel = face_vec(p, MAX_VEL * 8)
        d.vel[3] = 0
        d.entity = entity.load(nil, nil, t)
        pos[2] = pos[2]  - 12
        pos[3] = 8
        d.light = entity.load(nil, nil, {
            parent = p.entity, pos = pos,
            light = {type = Light.POINT, color = {1, .2, 0, 1}, att = 2048},
            anim = {
                light = {
                    rate_ms = 50,
                    f = {
                        type = AnimationFunction.RANDOM_F,
                        min = .8, max = 1,
                    },
                },
            },
        })
        d.remove = function()
            nngn:remove_entity(d.entity)
            nngn:remove_entity(d.light)
            d.entity, d.light = nil, nil
        end
    else
        if not d.entity then
            return
        end
        local p0 <const> = {p.entity:pos()}
        local p1 <const> = {d.entity:pos()}
        d.entity:set_parent(nil)
        d.entity:set_pos(p0[1] + p1[1], p0[2] + p1[2], 0)
        d.entity:set_vel(table.unpack(d.vel))
        entity.load(d.entity, nil, {
            collider = {
                type = Collider.AABB,
                bb = {-4, -8, 4, -4}, flags = Collider.TRIGGER,
            },
        })
        d.light:set_parent(d.entity)
        d.light:set_pos(0, -16, 8)
    end
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
    menu = player_menu,
    action = action,
    next = next,
    move = move,
    move_all = move_all,
    face_vec = face_vec,
    fairy = fairy,
    light = light,
    flashlight = flashlight,
    fire = fire,
}
