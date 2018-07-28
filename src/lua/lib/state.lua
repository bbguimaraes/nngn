local camera = require "nngn.lib.camera"

local save_camera
local function save(s)
    if s.camera ~= nil then
        save_camera(s)
    end
    if s.camera ~= nil or s.camera_follow ~= nil then
        s.camera_follow = camera.following() or false
    end
    if s.perspective ~= nil then
        s.perspective = nngn:camera():perspective()
    end
    if s.ambient_light ~= nil then
        s.ambient_light = {nngn:lighting():ambient_light()}
    end
    if s.shadows_enabled ~= nil then
        s.shadows_enabled = nngn:lighting():shadows_enabled()
    end
    if s.zsprites ~= nil then
        s.zsprites = nngn:renderers():zsprites()
    end
    return s
end

function save_camera(s)
    local c <const> = nngn:camera()
    s.perspective = c:perspective()
    s.camera_pos = {c:pos()}
    s.camera_rot = {c:rot()}
    s.camera_zoom = c:zoom()
    s.camera_limits = {c:bl_limit()}
    table.move({c:tr_limit()}, 1, 3, 4, s.camera_limits)
end

local function restore(s)
    if s.perspective ~= nil then
        nngn:camera():set_perspective(s.perspective)
        nngn:renderers():set_perspective(s.perspective)
    end
    if s.camera_pos ~= nil then
        nngn:camera():set_pos(table.unpack(s.camera_pos))
    end
    if s.camera_rot ~= nil then
        nngn:camera():set_rot(table.unpack(s.camera_rot))
    end
    if s.camera_zoom ~= nil then
        nngn:camera():set_zoom(s.camera_zoom)
    end
    if s.camera_limits ~= nil then
        nngn:camera():set_limits(table.unpack(s.camera_limits))
    end
    local camera_follow <const> = s.camera_follow
    if camera_follow then
        camera.set_follow(camera_follow)
    elseif camera_follow ~= nil then
        camera.set_follow(nil)
    end
    if s.ambient_light ~= nil then
        nngn:lighting():set_ambient_light(table.unpack(s.ambient_light))
    end
    if s.shadows_enabled ~= nil then
        nngn:lighting():set_shadows_enabled(s.shadows_enabled)
    end
    if s.zsprites ~= nil then
        nngn:renderers():set_zsprites(s.zsprites)
        nngn:lighting():set_zsprites(s.zsprites)
    end
end

return {
    save = save,
    restore = restore,
}
