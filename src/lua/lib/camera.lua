local nngn_math <const> = require "nngn.lib.math"
local utils <const> = require "nngn.lib.utils"

local PERSP_ANGLE <const> = math.rad(60)
local PERSP_ANGLE_SIN <const> = math.sin(PERSP_ANGLE)
local PERSP_ANGLE_COS <const> = math.cos(PERSP_ANGLE)

local MAX_VEL
local ROT_VEL
local ZOOM_VEL
local ROT_AXIS0 = 1
local ROT_AXIS1 = 3

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

local function set_rot_axes(a0, a1)
    ROT_AXIS0, ROT_AXIS1 = a0, a1
end

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
    local pos_y, pos_z, rot_x = 0, 0, 0
    if c:perspective() then
        rot_x = PERSP_ANGLE
        local z = c:fov_z() / zoom
        zoom = 1
        pos_y = -z * PERSP_ANGLE_SIN
        pos_z = z * PERSP_ANGLE_COS
    end
    c:set_pos(0, pos_y, pos_z)
    c:set_rot(rot_x, 0, 0)
    c:set_zoom(zoom)
end

local function set_perspective(p)
    nngn.renderers:set_perspective(p)
    nngn.camera:set_perspective(p)
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
    end
end

local function rotate(x, y)
    local c <const> = CAMERA
    local r <const> = nngn_math.vec3(c:rot())
    local r2 <const> = nngn_math.vec2(0x1p-7) * nngn_math.vec2(x, y)
    r[ROT_AXIS0] = r[ROT_AXIS0] - r2[2]
    r[ROT_AXIS1] = r[ROT_AXIS1] - r2[1]
    c:set_rot(table.unpack(r))
end

return {
    MAX_VEL = MAX_VEL,
    ROT_VEL = ROT_VEL,
    ZOOM_VEL = ZOOM_VEL,
    get = get,
    set = set,
    set_max_vel = set_max_vel,
    set_rot_axes = set_rot_axes,
    reset = reset,
    set_perspective = set_perspective,
    move = move,
    rotate = rotate,
}
