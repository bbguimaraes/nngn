FIRE_CMD_START = 0
FIRE_CMD_RELEASE = 1
FIRE_CMD_HIT = 2

local STATE_NONE = 0
local STATE_STARTED = 1
local STATE_RELEASED = 2
local STATE_EXPLOSION = 3

local LIGHT = {
    type = Light.POINT,
    color = {1, .2, 0, 1}, att = 2048}
local ANIM = {light = {
    rate_ms = 50, f = {
        type = AnimationFunction.RANDOM_F,
        min = .8, max = 1}}}
local COLLIDER0 = {
    type = Collider.AABB,
    bb = {-4, -8, 4, -4}, flags = Collider.TRIGGER}
local COLLIDER1 = {
    type = Collider.GRAVITY, flags = Collider.SOLID,
    m = -1e13, max_distance = 128}

local function remove(d) nngn:remove_entity(d.entity) end

local function cmd_start(p, d)
    if d.state == STATE_RELEASED then
        remove(d)
    elseif d.state and d.state ~= STATE_NONE then
        return
    end
    local t = dofile("src/lson/fire.lua")
    local pos = {p:face_vec(16)}
    pos[2] = pos[2] + p:entity():renderer():z_off() + 8
    t.pos = pos
    t.renderer.type = Renderer.TRANSLUCENT
    t.parent = p:entity()
    d.vel = {p:face_vec(PLAYER_MAX_VEL * 8)}
    d.vel[3] = 0
    d.entity = load_entity(nil, nil, t)
    pos[2] = pos[2]  - 12
    pos[3] = 8
    d.light = load_entity(nil, nil, {
        parent = p:entity(),
        pos = pos,
        light = LIGHT,
        anim = ANIM})
    d.state = STATE_STARTED
end

local function cmd_release(p, d)
    if d.state ~= STATE_STARTED then return end
    local p0 = {p:entity():pos()}
    local p1 = {d.entity:pos()}
    d.entity:set_parent(nil)
    d.entity:set_pos(p0[1] + p1[1], p0[2] + p1[2], 0)
    d.entity:set_vel(table.unpack(d.vel))
    load_entity(d.entity, nil, {collider = COLLIDER0})
    d.light:set_parent(d.entity)
    d.light:set_pos(0, -16, 8)
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
    load_entity(d.entity, nil, {collider = COLLIDER1})
    d.entity:set_vel(0, 0, 0)
    d.timer = 250
    d.heartbeat = nngn.schedule:next(
        Schedule.HEARTBEAT,
        function() fire(FIRE_CMD_END) end)
    d.state = STATE_EXPLOSION
end

local function cmd_end(p, d)
    d.timer = d.timer - nngn.timing:dt_ms()
    if d.timer > 0 then return end
    nngn.schedule:cancel(d.heartbeat)
    nngn.colliders:remove(d.entity:collider())
    d.entity:set_collider(collider)
    nngn:remove_entity(d.entity)
    d.timer = nil
    d.entity = nil
    d.state = STATE_NONE
end

function fire(cmd)
    local p = nngn.players:cur()
    if not p then return end
    local f
    if cmd == FIRE_CMD_START then f = cmd_start
    elseif cmd == FIRE_CMD_RELEASE then f = cmd_release
    elseif cmd == FIRE_CMD_HIT then f = cmd_hit
    elseif cmd == FIRE_CMD_END then f = cmd_end end
    if f then f(p, player_data(p, "fire")) end
end
