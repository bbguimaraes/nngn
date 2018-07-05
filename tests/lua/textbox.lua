dofile "src/lua/path.lua"
local common = require "tests/lua/common"
local textbox = require "nngn.lib.textbox"

local function test_update()
    textbox.update("title", "text")
    common.assert_eq(nngn.textbox:title(), "title")
    common.assert_eq(nngn.textbox:text(), "text")
    common.assert_eq(nngn.textbox:speed(), textbox.SPEED_NORMAL)
end

local function test_clear()
    textbox.update("title", "text")
    textbox.clear()
    common.assert_eq(nngn.textbox:title(), "")
    common.assert_eq(nngn.textbox:text(), "")
end

common.setup_hook(1)
test_update()
test_clear()
nngn:exit()
