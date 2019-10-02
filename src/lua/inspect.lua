local inspect = require "nngn.lib.inspect"
local tools = require "nngn.lib.tools"

local function named(name)
    local f = inspect.FS[name]
    if not f then error("invalid name: " .. name) end
    return inspect.inspect(f)
end

return setmetatable({
    inspect = tools.wrap(inspect.inspect),
    named = named,
}, {__index = inspect})
