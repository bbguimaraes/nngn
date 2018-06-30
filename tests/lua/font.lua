dofile "src/lua/path.lua"
local common = require "tests/lua/common"
local font = require "nngn.lib.font"

local function test_load()
    local fonts_enabled
    do
        local f = nngn.fonts:load(4, "tests/graphics/texture_test.png")
        if f ~= 0 then fonts_enabled = true end
    end
    font.load(4, "DejaVuSans.ttf")
    if fonts_enabled then common.assert_eq(nngn.fonts:n(), 2) end
end

common.setup_hook(1)
test_load()
nngn:exit()
