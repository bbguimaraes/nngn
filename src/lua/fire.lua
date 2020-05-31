FIRE_CMD_START = 0
FIRE_CMD_RELEASE = 1
FIRE_CMD_HIT = 2

local STATE_NONE <const> = 0
local STATE_STARTED <const> = 1
local STATE_RELEASED <const> = 2
local STATE_EXPLOSION <const> = 3

local FNS

local LIGHT <const> = {
    type = Light.POINT,
    color = {1, .2, 0, 1},
    att = 2048,
}

local ANIM <const> = {
    light = {
        rate_ms = 50,
        f = {
            type = AnimationFunction.RANDOM_F,
            min = .8, max = 1,
        },
    },
}

local COLLIDER0 <const> = {
    type = Collider.AABB,
    bb = {-4, -8, 4, -4},
    flags = Collider.TRIGGER,
}

local COLLIDER1 <const> = {
    type = Collider.GRAVITY,
    flags = Collider.SOLID,
    m = -1e13,
    max_distance = 128,
}

local function remove(d) nngn:remove_entity(d.entity) end

local function cmd_start(p, d)
    if d.state == STATE_RELEASED then
        remove(d)
    elseif d.state and d.state ~= STATE_NONE then
        return
    end
    local t <const> = dofile("src/lson/zelda/fire.lua")
    local pos <const> = player.face_vec(p, 16)
    pos[2] = pos[2] + p.entity:renderer():z_off() + 8
    t.pos = pos
    t.renderer.type = Renderer.TRANSLUCENT
    t.parent = p.entity
    d.vel = player.face_vec(p, player.MAX_VEL * 8)
    d.vel[3] = 0
    d.entity = entity.load(nil, nil, t)
    pos[2] = pos[2]  - 12
    pos[3] = 8
    d.light = entity.load(nil, nil, {
        parent = p.entity,
        pos = pos,
        light = LIGHT,
        anim = ANIM,
    })
    d.state = STATE_STARTED
end

local function cmd_release(p, d)
    if d.state ~= STATE_STARTED then
        return
    end
    local p0 <const> = {p.entity:pos()}
    local p1 <const> = {d.entity:pos()}
    d.entity:set_parent(nil)
    nngn:entities():set_pos(d.entity, p0[1] + p1[1], p0[2] + p1[2], 0)
    d.entity:set_vel(table.unpack(d.vel))
    entity.load(d.entity, nil, {collider = COLLIDER0})
    d.light:set_parent(d.entity)
    nngn:entities():set_pos(d.light, 0, -16, 8)
    d.state = STATE_RELEASED
end

local function cmd_hit(p, d)
    if d.state == STATE_STARTED then
        -- TODO
    elseif d.state == STATE_RELEASED then
        -- TODO
        nngn.colliders:remove(d.entity:collider())
    else
        return
    end
    nngn:remove_entity(d.light)
    d.light = nil
    entity.load(d.entity, nil, {collider = COLLIDER1})
    d.entity:set_vel(0, 0, 0)
    d.timer = 250
    d.heartbeat = nngn.schedule:next(
        Schedule.HEARTBEAT,
        function() fire(FIRE_CMD_END) end)
    d.state = STATE_EXPLOSION
end

local function cmd_end(p, d)
    d.timer = d.timer - nngn.timing:dt_ms()
    if d.timer > 0 then
        return
    end
    nngn.schedule:cancel(d.heartbeat)
    nngn.colliders:remove(d.entity:collider())
    d.entity:set_collider(collider)
    nngn:remove_entity(d.entity)
    d.timer = nil
    d.entity = nil
    d.state = STATE_NONE
end

FNS = {
    FIRE_CMD_START = 0
    FIRE_CMD_RELEASE = 1
    FIRE_CMD_HIT = 2
}

function fire(cmd)
    local p <const> = player.cur()
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
    local f <const> = FNS[cmd]
    if f then
        f(p)
    end
end
