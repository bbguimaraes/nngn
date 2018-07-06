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

return {
    test_pprint,
}
