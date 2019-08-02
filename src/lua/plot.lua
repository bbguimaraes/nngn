local plot = require "nngn.lib.plot"
local tools = require "nngn.lib.tools"

local function named(name)
    local f = plot.FS[name]
    if not f then error("invalid name: " .. name) end
    return plot.plot(f)
end

return setmetatable({
    plot = tools.wrap(plot.plot),
    named = tools.wrap(named),
    func = tools.wrap(plot.func),
    eval = tools.wrap(plot.eval),
}, {__index = plot})
