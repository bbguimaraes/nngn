local configure = require "nngn.lib.configure"
local tools = require "nngn.lib.tools"

local function named(name)
    local f = configure.FS[name]
    if not f then error("invalid name: " .. name) end
    return configure.configure(f)
end

return setmetatable({
    configure = tools.wrap(configure.configure),
    named = tools.wrap(named),
}, {__index = configure})
