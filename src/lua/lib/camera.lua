local utils = require "nngn.lib.utils"

local MAX_VEL
local ROT_VEL
local ZOOM_VEL
local ANGLE = math.rad(60)
local ANGLE_SIN = math.sin(ANGLE)
local ANGLE_COS = math.cos(ANGLE)
local FOLLOW = nil

local CAMERA = nngn.camera

local function get() return CAMERA end

local function set(c)
    local ret = CAMERA
    CAMERA = c
    return ret
end

local function set_max_vel(v)
    MAX_VEL = v
    ROT_VEL = MAX_VEL / 16
    ZOOM_VEL = MAX_VEL / 64
    local c = CAMERA
    c:set_max_vel(MAX_VEL)
    c:set_max_rot_vel(ROT_VEL)
    c:set_max_zoom_vel(ZOOM_VEL)
end
set_max_vel(64)

local function reset(zoom)
    local c = CAMERA
    zoom = zoom or 2
    c:set_vel(0, 0, 0)
    c:set_max_vel(MAX_VEL)
    c:set_damp(5)
    c:set_acc(0, 0, 0)
    c:set_rot_vel(0, 0, 0)
    c:set_rot_acc(0, 0, 0)
    c:set_zoom_vel(0)
    c:set_zoom_acc(0)
    local pos_x, pos_y, pos_z, rot_x = 0, 0, 0, 0
    if FOLLOW then
        local fp = {FOLLOW:pos()}
        pos_x, pos_y = fp[1], fp[2]
    end
    if c:perspective() then
        rot_x = ANGLE
        local z = c:fov_z() / zoom
        zoom = 1
        pos_y = -z * ANGLE_SIN
        pos_z = z * ANGLE_COS
    end
    c:set_pos(pos_x, pos_y, pos_z)
    c:set_rot(rot_x, 0, 0)
    c:set_zoom(zoom)
end

local function following() return FOLLOW end

local function set_follow(e)
    local c = CAMERA
    if FOLLOW then FOLLOW:set_camera(nil) end
    FOLLOW = e
    if e then e:set_camera(c) end
end

local function toggle_follow()
    if FOLLOW then set_follow(nil)
    elseif nngn.players:n() > 0 then
        set_follow(nngn.players:cur():entity())
    end
end

local function move(key, press, mods, keys)
    local c = CAMERA
    local shift = mods & Input.MOD_SHIFT ~= 0
    if press and utils.check_double_tap(nngn.timing:now_ms(), key, shift) then
        c:set_dash(true)
    end
    keys = keys or nngn.input:get_keys{
        Input.KEY_LEFT, Input.KEY_RIGHT, Input.KEY_DOWN, Input.KEY_UP,
        Input.KEY_PAGE_DOWN, Input.KEY_PAGE_UP}
    local x = keys[2] - keys[1]
    local y = keys[4] - keys[3]
    local z = keys[6] - keys[5]
    local rx, ry, rz, ax, ay, az
    if shift then
        local a = ROT_VEL
        local rx, ry, rz = c:rot_acc()
        ry = x * -a
        rx = y * -a
        rz = z * -a
        c:set_rot_acc(rx, ry, rz)
        if rx == 0 and ry == 0 and rz == 0 then c:set_dash(false) end
    else
        local a = 10 * MAX_VEL
        local ax, ay, az = c:acc()
        local zoom_a = c:zoom_acc()
        ax = x * a
        ay = y * a
        if mods & Input.MOD_ALT == 0 then az = z * a
        else zoom_a = z * ZOOM_VEL end
        c:set_acc(ax, ay, az)
        c:set_zoom_acc(zoom_a)
        if ax == 0 and ay == 0 and az == 0 and zoom_a == 0 then
            c:set_dash(false)
        end
        if FOLLOW and (x or y) then set_follow(nil) end
    end
end

return {
    MAX_VEL = MAX_VEL,
    ROT_VEL = ROT_VEL,
    ZOOM_VEL = ZOOM_VEL,
    get = get,
    set = set,
    set_max_vel = set_max_vel,
    reset = reset,
    following = following,
    toggle_follow = toggle_follow,
    set_follow = set_follow,
    move = move,
}
