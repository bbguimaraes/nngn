dofile "src/lua/path.lua"
local common = require "tests.lua.common"

common.setup_hook(1)
for _, f in ipairs{
    "tests/lua/utils.lua",
    "tests/lua/math.lua",
} do
    for _, t in ipairs(dofile(f)) do
        t()
    end
end
