local common = require "tests.lua.common"
local utils = require "nngn.lib.utils"

local function test_pprint()
    for _, t in ipairs{
        {0, "0\n"},
        {"0", '"0"\n'},
        {{}, "{}\n"},
        {{0}, "1: 0\n"},
        {{"0"}, '1: "0"\n'},
        {{0, 1, 2}, "1: 0\n2: 1\n3: 2\n"},
        {{0, {1, 2}}, "1: 0\n2:\n  1: 1\n  2: 2\n"},
        {{0, {1, {2}}}, "1: 0\n2:\n  1: 1\n  2:\n    1: 2\n"},
    } do
        local input, expected = table.unpack(t)
        local output = common.wrap_stdout(function() utils.pprint(input) end)
        common.assert_eq(output, expected)
    end
end

local function test_fmt_time()
    for _, t in ipairs{
        {1.0,    "   1.000ns"},
        {1.5,    "   1.500ns"},
        {1.0e3,  "   1.000µs"},
        {1.5e3,  "   1.500µs"},
        {1.0e6,  "   1.000ms"},
        {1.5e9,  "   1.500s"},
        {1.0e12, "1000.000s"},
    } do
        local input, expected = table.unpack(t)
        common.assert_eq(utils.fmt_time(input), expected)
    end
end

local function test_fmt_size()
    for _, t in ipairs{
        {                        1,    "1.0"},
        {                      1.5,    "1.5"},
        {                     1024,    "1.0K"},
        {               1.5 * 1024,    "1.5K"},
        {              1024 * 1024,    "1.0M"},
        {        1.5 * 1024 * 1024,    "1.5M"},
        {       1024 * 1024 * 1024,    "1.0G"},
        {1000 * 1024 * 1024 * 1024, "1000.0G"},
    } do
        local input, expected = table.unpack(t)
        common.assert_eq(utils.fmt_size(input), expected)
    end
end

local function test_shift()
    common.assert_eq(utils.shift(1, 3, true, 1), 2)
    common.assert_eq(utils.shift(2, 3, true, 1), 3)
    common.assert_eq(utils.shift(3, 3, true, 1), 1)
    common.assert_eq(utils.shift(1, 3, false, 1), 3)
    common.assert_eq(utils.shift(2, 3, false, 1), 1)
    common.assert_eq(utils.shift(3, 3, false, 1), 2)
    common.assert_eq(utils.shift(2, 3, true, 2), 3)
    common.assert_eq(utils.shift(3, 3, true, 2), 4)
    common.assert_eq(utils.shift(4, 3, true, 2), 2)
end

local function test_double_tap()
    local key = string.byte(" ")
    local interval = utils.get_double_tap_interval()
    utils.set_double_tap_interval(5)
    utils.reset_double_tap(key)
    assert(not utils.check_double_tap(10, key))
    assert(utils.check_double_tap(15, key))
    assert(utils.check_double_tap(15, key))
    assert(not utils.check_double_tap(15, key, true))
    assert(not utils.check_double_tap(30, key, true))
    assert(not utils.check_double_tap(30, key))
    utils.reset_double_tap(key)
    utils.reset_double_tap(key, true)
    utils.set_double_tap_interval(interval)
end

local function test_find()
    local t <const> = {0, 1, 3, 4, 5}
    common.assert_eq(utils.find(t, 6), nil)
    common.assert_eq(utils.find(t, 0), 1)
    common.assert_eq(utils.find(t, 1), 2)
    common.assert_eq(utils.find(t, 2), nil)
    common.assert_eq(utils.find(t, 3), 3)
    common.assert_eq(utils.find(t, 4), 4)
    common.assert_eq(utils.find(t, 5), 5)
end

return {
    test_pprint,
    test_fmt_time,
    test_fmt_size,
    test_shift,
    test_double_tap,
    test_find,
}
