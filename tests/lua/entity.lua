dofile "src/lua/path.lua"
local entity = require "nngn.lib.entity"
local common = require "tests/lua/common"

local function test_load()
    common.assert_eq(nngn:entities():n(), 0)
    local e = entity.load()
    common.assert_eq(nngn:entities():n(), 1)
    common.assert_eq(nngn:entities():name(e), "")
    common.assert_eq(nngn:entities():tag(e), "")
    common.assert_eq({e:pos()}, {0, 0, 0}, common.deep_cmp)
    common.assert_eq(e:renderer(), nil)
    entity.load(e, nil, {name = "name"})
    common.assert_eq(nngn:entities():n(), 1)
    common.assert_eq(nngn:entities():name(e), "name")
    entity.load(e, nil, {tag = "tag"})
    common.assert_eq(nngn:entities():n(), 1)
    common.assert_eq(nngn:entities():tag(e), "tag")
    entity.load(e, nil, {pos = {0, 1, 2}})
    common.assert_eq(nngn:entities():n(), 1)
    common.assert_eq({e:pos()}, {0, 1, 2}, common.deep_cmp)
    entity.load(e, nil, {
        renderer = {
            type = Renderer.SPRITE, size = {0, 0}}})
    common.assert_eq(nngn:entities():n(), 1)
    assert(e:renderer() ~= nil)
    nngn:remove_entity(e)
end

local function test_by_name()
    local e0 = entity.load(nil, nil, {name = "e0"})
    local e1 = entity.load(nil, nil, {name = "e1"})
    common.assert_eq(nngn:entities():by_name("e0"), {e0}, common.deep_cmp)
    common.assert_eq(nngn:entities():by_name("e1"), {e1}, common.deep_cmp)
    nngn:remove_entity(e1)
    nngn:remove_entity(e0)
end

local function test_by_name_hash()
    local e0 = entity.load(nil, nil, {name = "e0"})
    local e1 = entity.load(nil, nil, {name = "e1"})
    local h0, h1 = Math.hash("e0"), Math.hash("e1")
    common.assert_eq(nngn:entities():by_name_hash(h0), {e0}, common.deep_cmp)
    common.assert_eq(nngn:entities():by_name_hash(h1), {e1}, common.deep_cmp)
    nngn:remove_entity(e1)
    nngn:remove_entity(e0)
end

local function test_by_tag()
    local e0 = entity.load(nil, nil, {tag = "t0"})
    local e1 = entity.load(nil, nil, {tag = "t0"})
    local e2 = entity.load(nil, nil, {tag = "t1"})
    common.assert_eq(nngn:entities():by_tag("t0"), {e0, e1}, common.deep_cmp)
    common.assert_eq(nngn:entities():by_tag("t1"), {e2}, common.deep_cmp)
    nngn:remove_entity(e2)
    nngn:remove_entity(e1)
    nngn:remove_entity(e0)
end

local function test_by_tag_hash()
    local e0 = entity.load(nil, nil, {tag = "t0"})
    local e1 = entity.load(nil, nil, {tag = "t0"})
    local e2 = entity.load(nil, nil, {tag = "t1"})
    local h0, h1 = Math.hash("t0"), Math.hash("t1")
    common.assert_eq(nngn:entities():by_tag_hash(h0), {e0, e1}, common.deep_cmp)
    common.assert_eq(nngn:entities():by_tag_hash(h1), {e2}, common.deep_cmp)
    nngn:remove_entity(e2)
    nngn:remove_entity(e1)
    nngn:remove_entity(e0)
end

assert(nngn:set_graphics(Graphics.PSEUDOGRAPH))
nngn:entities():set_max(3)
nngn:renderers():set_max_sprites(1)
common.setup_hook(1)
test_load()
test_by_name()
test_by_name_hash()
test_by_tag()
test_by_tag_hash()
nngn:exit()
