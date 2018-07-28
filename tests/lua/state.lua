dofile "src/lua/path.lua"
local common = require "tests/lua/common"
local camera = require "nngn.lib.camera"
local state = require "nngn.lib.state"

local function test_camera()
    local c <const> = nngn:camera()
    c:set_perspective(true)
    c:look_at(0, 0, 0, 1, 2, 3, 0, 1, 0)
    c:set_rot(4, 5, 6)
    c:set_zoom(7)
    c:set_limits(8, 9, 10, 11, 12, 13)
    local s <const> = state.save{camera = true}
    common.assert_eq({c:pos()}, {1, 2, 3}, common.deep_cmp)
    common.assert_eq({c:rot()}, {4, 5, 6}, common.deep_cmp)
    common.assert_eq(c:zoom(), 7)
    common.assert_eq({c:bl_limit()}, {8, 9, 10}, common.deep_cmp)
    common.assert_eq({c:tr_limit()}, {11, 12, 13}, common.deep_cmp)
    assert(c:perspective())
    c:set_perspective(false)
    c:set_pos(0, 0, 0)
    c:set_rot(0, 0, 0)
    c:set_zoom(0)
    c:set_limits(0, 0, 0, 0, 0, 0)
    state.restore(s)
    common.assert_eq({c:pos()}, {1, 2, 3}, common.deep_cmp)
    common.assert_eq({c:rot()}, {4, 5, 6}, common.deep_cmp)
    common.assert_eq(c:zoom(), 7)
    common.assert_eq({c:bl_limit()}, {8, 9, 10}, common.deep_cmp)
    common.assert_eq({c:tr_limit()}, {11, 12, 13}, common.deep_cmp)
    assert(c:perspective())
end

local function test_camera_follow_unset()
    camera.set_follow(nil)
    local s <const> = state.save{camera_follow = true}
    common.assert_eq(camera.following(), nil)
    local e <const> = nngn:entities():add()
    camera.set_follow(e)
    state.restore(s)
    common.assert_eq(camera.following(), nil)
    nngn:remove_entity(e)
end

local function test_camera_follow_set()
    local e <const> = nngn:entities():add()
    camera.set_follow(e)
    local s <const> = state.save{camera_follow = true}
    common.assert_eq(camera.following(), e)
    camera.set_follow(nil)
    state.restore(s)
    common.assert_eq(camera.following(), e)
    camera.set_follow(nil)
    nngn:remove_entity(e)
end

local function test_perspective_unset()
    nngn:camera():set_perspective(false)
    nngn:renderers():set_perspective(false)
    local s <const> = state.save{perspective = true}
    assert(not nngn:camera():perspective())
    assert(not nngn:renderers():perspective())
    nngn:camera():set_perspective(true)
    nngn:renderers():set_perspective(true)
    state.restore(s)
    assert(not nngn:camera():perspective())
    assert(not nngn:renderers():perspective())
end

local function test_perspective_set()
    nngn:camera():set_perspective(true)
    nngn:renderers():set_perspective(true)
    local s <const> = state.save{perspective = true}
    assert(nngn:camera():perspective())
    assert(nngn:renderers():perspective())
    nngn:camera():set_perspective(false)
    nngn:renderers():set_perspective(false)
    state.restore(s)
    assert(nngn:camera():perspective())
    assert(nngn:renderers():perspective())
end

local function test_ambient_light()
    local l <const> = nngn:lighting()
    l:set_ambient_light(1, 2, 3, 4)
    local s <const> = state.save{ambient_light = true}
    common.assert_eq({l:ambient_light()}, {1, 2, 3, 4}, common.deep_cmp)
    l:set_ambient_light(0, 0, 0, 0)
    state.restore(s)
    common.assert_eq({l:ambient_light()}, {1, 2, 3, 4}, common.deep_cmp)
    l:set_ambient_light(1, 1, 1, 1)
end

local function test_shadows_enabled()
    local l <const> = nngn:lighting()
    l:set_shadows_enabled(true)
    local s <const> = state.save{shadows_enabled = true}
    assert(l:shadows_enabled())
    l:set_shadows_enabled(false)
    state.restore(s)
    assert(l:shadows_enabled())
    l:set_shadows_enabled(false)
end

local function test_shadows_disabled()
    local l <const> = nngn:lighting()
    l:set_shadows_enabled(false)
    local s <const> = state.save{shadows_enabled = true}
    assert(not l:shadows_enabled())
    l:set_shadows_enabled(true)
    state.restore(s)
    assert(not l:shadows_enabled())
end

local function test_zsprites_unset()
    local r <const>, l <const> = nngn:renderers(), nngn:lighting()
    r:set_zsprites(false)
    l:set_zsprites(false)
    local s <const> = state.save{zsprites = true}
    assert(not r:zsprites())
    r:set_zsprites(true)
    l:set_zsprites(true)
    state.restore(s)
    assert(not r:zsprites())
    assert(not l:zsprites())
end

local function test_zsprites_set()
    local r <const>, l <const> = nngn:renderers(), nngn:lighting()
    r:set_zsprites(true)
    l:set_zsprites(true)
    local s <const> = state.save{zsprites = true}
    assert(r:zsprites())
    r:set_zsprites(false)
    l:set_zsprites(false)
    state.restore(s)
    assert(r:zsprites())
    assert(l:zsprites())
    r:set_zsprites(false)
    l:set_zsprites(false)
end

nngn:entities():set_max(2)
common.setup_hook(1)
test_camera()
test_camera_follow_unset()
test_camera_follow_set()
test_perspective_unset()
test_perspective_set()
test_ambient_light()
test_shadows_enabled()
test_shadows_disabled()
nngn:exit()
