local camera = require "nngn.lib.camera"

local function save(s)
    if s.camera_follow ~= nil then s.camera_follow = camera.following() end
    if s.perspective ~= nil
    then s.perspective = nngn.camera:perspective() end
    if s.ambient_light ~= nil
    then s.ambient_light = {nngn.lighting:ambient_light()} end
    if s.shadows_enabled ~= nil
    then s.shadows_enabled = nngn.lighting:shadows_enabled() end
    if s.zsprites ~= nil
    then s.zsprites = nngn.renderers:zsprites() end
    return s
end

local function restore(s)
    if s.camera_follow ~= nil then camera.set_follow(s.camera_follow) end
    if s.perspective ~= nil then
        nngn.camera:set_perspective(s.perspective)
        nngn.renderers:set_perspective(s.perspective)
    end
    if s.ambient_light ~= nil then
        nngn.lighting:set_ambient_light(table.unpack(s.ambient_light))
    end
    if s.shadows_enabled ~= nil then
        nngn.lighting:set_shadows_enabled(s.shadows_enabled)
    end
    if s.zsprites ~= nil then
        nngn.renderers:set_zsprites(s.zsprites)
        nngn.lighting:set_zsprites(s.zsprites)
    end
end

return {
    save = save,
    restore = restore,
}
