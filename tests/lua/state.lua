dofile "src/lua/path.lua"
local common = require "tests/lua/common"
local camera = require "nngn.lib.camera"
local state = require "nngn.lib.state"

local function test_save_restore()
    local e = nngn.entities:add()
    camera.set_follow(e)
    nngn.camera:set_perspective(true)
    nngn.lighting:set_ambient_light(1, 2, 3, 4)
    nngn.lighting:set_shadows_enabled(true)
    nngn.renderers:set_zsprites(true)
    local s = state.save{
        camera_follow = true, perspective = true, ambient_light = true,
        shadows_enabled = true, zsprites = true}
    common.assert_eq(s.camera_follow, e)
    common.assert_eq(s.perspective, true)
    common.assert_eq(s.ambient_light, {1, 2, 3, 4}, common.deep_cmp)
    common.assert_eq(s.shadows_enabled, true)
    common.assert_eq(s.zsprites, true)
    camera.set_follow(nil)
    nngn.camera:set_perspective(false)
    nngn.renderers:set_perspective(false)
    nngn.lighting:set_ambient_light(0, 0, 0, 0)
    nngn.lighting:set_shadows_enabled(false)
    nngn.renderers:set_zsprites(false)
    state.restore(s)
    common.assert_eq(camera.following(), e)
    common.assert_eq(nngn.camera:perspective(), true)
    common.assert_eq(nngn.renderers:perspective(), true)
    common.assert_eq(
        {nngn.lighting:ambient_light()}, {1, 2, 3, 4}, common.deep_cmp)
    common.assert_eq(nngn.lighting:shadows_enabled(), true)
    common.assert_eq(nngn.renderers:zsprites(), true)
    common.assert_eq(nngn.lighting:zsprites(), true)
    nngn:remove_entity(e)
end

nngn.entities:set_max(1)
common.setup_hook(1)
test_save_restore()
nngn:exit()
