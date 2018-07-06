local common <const> = require "tests.lua.common"
local serial <const> = require "nngn.lib.serial"

local function deserialize(s)
    return assert(load("return " .. s))()
end

local function test_empty()
    common.assert_eq(serial.serialize{}, "{}")
    common.assert_eq(deserialize(serial.serialize{}), {}, common.deep_cmp)
end

local function test_table()
    local t <const> = {
        a = 42,
        t = {true, "test", [0.1] = math.pi},
    }
    common.assert_eq(deserialize(serial.serialize(t)), t, common.deep_cmp)
end

local function test_indent()
    local t <const> = {
        a = 42,
        t = {true, "test", [0.1] = math.pi},
    }
    common.assert_eq(deserialize(serial.serialize(t, 2)), t, common.deep_cmp)
end

return {
    test_empty,
    test_table,
    test_indent,
}
