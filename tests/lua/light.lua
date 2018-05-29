dofile "src/lua/path.lua"
local common = require "tests/lua/common"
local light = require "nngn.lib.light"
local nngn_math = require "nngn.lib.math"

local function test_sun()
    assert(not nngn.lighting:sun_light())
    light.sun(false)
    assert(not nngn.lighting:sun_light())
    light.sun()
    local sun = nngn.lighting:sun()
    common.assert_eq(sun:incidence(), math.rad(315), function(x, y)
        return nngn_math.float_eq(x, y, 2 * Math.FLOAT_EPSILON)
    end)
    common.assert_eq(sun:time_ms(), 21600000)
    local l = nngn.lighting:sun_light()
    assert(l)
    local color = {l:color()}
    common.assert_eq(color[1], 1)
    common.assert_eq(color[2], 0.8, nngn_math.float_eq)
    common.assert_eq(color[3], 0.5)
    common.assert_eq(color[4], 1)
    light.sun(true)
    assert(nngn.lighting:sun_light())
    light.sun()
    assert(not nngn.lighting:sun_light())
    common.assert_eq(nngn.entities:n(), 0)
end

common.setup_hook(1)
test_sun()
nngn:exit()
