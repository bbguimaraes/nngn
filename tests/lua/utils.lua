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

return {
    test_pprint,
    test_fmt_time,
}
