local utils = require "nngn.lib.utils"

local DIR = "img"
local DEFAULT = DIR .. "/default.png"
local NNGN = DIR .. "/nngn.png"

local guard = utils.class {
    new = function(self, tex)
        local ret = {tex = tex}
        if type(tex) == "string" then
            tex = nngn:textures():load(tex)
            ret.tex = tex
            ret.remove = tex
        end
        return setmetatable(ret, self)
    end,
    __close = function(t)
        local tex = t.remove
        if tex then nngn:textures():remove(tex) end
    end,
}

local function set_max(n)
    nngn:textures():set_max(n)
    local g = nngn:graphics()
    if g then g:resize_textures(n) end
end

local function load(tex) return guard:new(tex) end

return {
    DIR = DIR,
    DEFAULT = DEFAULT,
    NNGN = NNGN,
    guard = guard,
    set_max = set_max,
    load = load,
}
