require "src/lua/path"
local font <const> = require "nngn.lib.font"

require("nngn.lib.graphics").init()
require("nngn.lib.input").install()
font.load(32)

local s = ""
for i = 32, 127 do
    s = s .. string.format("%x:%s", i, string.char(i))
    if i & 0x3 ~= 0x3 then
        s = s .. " "
    elseif i ~= 127 then
        s = s .. "\n"
    end
end
nngn:renderers():set_max_text(#font.DEFAULT + #s)
nngn:textbox():set_monospaced(true)
nngn:textbox():set_speed(0)
require("nngn.lib.textbox").update(font.DEFAULT, s)
function demo_start() end
