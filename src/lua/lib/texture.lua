local utils = require "nngn.lib.utils"

local DIR = "img"
local DEFAULT = DIR .. "/default.png"
local NNGN = DIR .. "/nngn.png"

local function set_max(n)
    nngn:textures():set_max(n)
    local g = nngn:graphics()
    if g then g:resize_textures(n) end
end

return {
    DIR = DIR,
    DEFAULT = DEFAULT,
    NNGN = NNGN,
    set_max = set_max,
}
