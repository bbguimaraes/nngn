local common = require "tests/lua/common"
local nngn_math = require "nngn.lib.math"

local e = 1.19e-07
local float_cmp = function(x, y) return nngn_math.float_eq(x, y, e) end

local function test_float_eq()
    assert(nngn_math.float_eq(0, 0, e))
    assert(nngn_math.float_eq(1, 1, e))
    assert(nngn_math.float_eq(1, 1.5, 1))
    assert(not nngn_math.float_eq(1, 1.5, e), .5)
end

local function test_clamp()
    common.assert_eq(nngn_math.clamp(-1,   0, 1), 0)
    common.assert_eq(nngn_math.clamp( 0,   0, 1), 0)
    common.assert_eq(nngn_math.clamp( 0.5, 0, 1), 0.5)
    common.assert_eq(nngn_math.clamp( 1,   0, 1), 1)
    common.assert_eq(nngn_math.clamp( 2,   0, 1), 1)
end

local function test_vec2()
    common.assert_eq(nngn_math.vec2(), {0, 0}, common.deep_cmp)
    common.assert_eq(nngn_math.vec2(1), {1, 1}, common.deep_cmp)
    common.assert_eq(nngn_math.vec2(0, 1), {0, 1}, common.deep_cmp)
    local v0, v1 = nngn_math.vec2(1, 2), nngn_math.vec2(3, 4)
    common.assert_eq(v0 + v1, {4, 6}, common.deep_cmp)
    common.assert_eq(v0 - v1, {-2, -2}, common.deep_cmp)
    common.assert_eq(v0 * v1, {3, 8}, common.deep_cmp)
    common.assert_eq(v0 / v1, {1/3, 0.5}, common.deep_cmp)
    common.assert_eq(v0 // v1, {0, 0}, common.deep_cmp)
end

local function test_vec3()
    common.assert_eq(nngn_math.vec3(), {0, 0, 0}, common.deep_cmp)
    common.assert_eq(nngn_math.vec3(1), {1, 1, 1}, common.deep_cmp)
    common.assert_eq(nngn_math.vec3(0, 1), {0, 1, 0}, common.deep_cmp)
    common.assert_eq(nngn_math.vec3(0, 1, 2), {0, 1, 2}, common.deep_cmp)
    local v0, v1 = nngn_math.vec3(1, 2, 3), nngn_math.vec3(4, 5, 6)
    common.assert_eq(v0 + v1, {5, 7, 9}, common.deep_cmp)
    common.assert_eq(v0 - v1, {-3, -3, -3}, common.deep_cmp)
    common.assert_eq(v0 * v1, {4, 10, 18}, common.deep_cmp)
    common.assert_eq(v0 / v1, {0.25, 0.4, 0.5}, common.deep_cmp)
    common.assert_eq(v0 // v1, {0, 0, 0}, common.deep_cmp)
end

local function test_dot()
    common.assert_eq(nngn_math.vec2(1, 2):dot(nngn_math.vec2(3, 4)), 11)
    common.assert_eq(nngn_math.vec3(1, 2, 3):dot(nngn_math.vec3(4, 5, 6)), 32)
end

local function test_len()
    local v = nngn_math.vec2(1, 2)
    common.assert_eq(v:len_sq(), 5)
    common.assert_eq(v:len(), math.sqrt(5))
    v = nngn_math.vec3(1, 2, 3)
    common.assert_eq(v:len_sq(), 14)
    common.assert_eq(v:len(), math.sqrt(14))
end

local function test_norm()
    local l = math.sqrt(5)
    common.assert_eq(
        nngn_math.vec2(1, 2):norm(), {1 / l, 2 / l}, common.deep_cmp)
    l = math.sqrt(14)
    common.assert_eq(
        nngn_math.vec3(1, 2, 3):norm(), {1 / l, 2 / l, 3 / l}, common.deep_cmp)
end

local function test_cross()
    common.assert_eq(
        nngn_math.vec3(1, 2, 3):cross(nngn_math.vec3(4, 5, 6)),
        {-3, 6, -3},
        common.deep_cmp)
end

local function test_sqrt()
    local cmp = {1, math.sqrt(2)}
    common.assert_eq(nngn_math.vec2(1, 2):sqrt(), cmp, common.deep_cmp)
    cmp[3] = math.sqrt(3)
    common.assert_eq(nngn_math.vec3(1, 2, 3):sqrt(), cmp, common.deep_cmp)
end

local function test_reflect()
    local r = nngn_math.vec2(0, 1):reflect(nngn_math.vec2(1):norm())
    common.assert_eq(r[1], 1.0, float_cmp)
    common.assert_eq(r[2], 0.0, float_cmp)
    local sq2_2 = math.sqrt(2) / 2
    r = nngn_math.vec3(0, 1, 0):reflect(nngn_math.vec3(0.5, sq2_2, 0.5))
    common.assert_eq(r[1], sq2_2, float_cmp)
    common.assert_eq(r[2], 0.0, float_cmp)
    common.assert_eq(r[3], sq2_2, float_cmp)
end

local function test_refract()
    local v = nngn_math.vec2(1, 1):norm()
    local n = nngn_math.vec2(-1, 0)
    local r = v:refract(n, 1, 1)
    common.assert_eq(r[1], v[1], float_cmp)
    common.assert_eq(r[2], v[2], float_cmp)
    r = v:refract(n, 1.5, 2)
    common.assert_eq(r[1], 1/3, float_cmp)
    common.assert_eq(r[2], v[2] / 0.75, float_cmp)
    r = v:refract(n, 2, 1.5)
    common.assert_eq(r[1], math.sqrt(1 - (.75 * .75) / 2), float_cmp)
    common.assert_eq(r[2], v[2] * 0.75, float_cmp)
end

local function test_interp()
    local v = nngn_math.vec2(0.5, 1)
    local u = nngn_math.vec2(1, 2)
    common.assert_eq(v:interp(u, 0), v, common.deep_cmp)
    common.assert_eq(v:interp(u, 1), u, common.deep_cmp)
    common.assert_eq(v:interp(u, 0.5), {0.75, 1.5}, common.deep_cmp)
end

local function test_str()
    common.assert_eq(
        nngn_math.vec2(1, 2):str(),
        "{1.000000, 2.000000}")
    common.assert_eq(
        nngn_math.vec3(1, 2, 3):str(),
        "{1.000000, 2.000000, 3.000000}")
end

return {
    test_float_eq,
    test_clamp,
    test_vec2,
    test_vec3,
    test_dot,
    test_len,
    test_norm,
    test_cross,
    test_sqrt,
    test_reflect,
    test_refract,
    test_interp,
    test_str,
}
