local profile = require "nngn.lib.profile"
local timeline = require "nngn.lib.timeline"
local tools = require "nngn.lib.tools"

local stats = {
    profile = Profile,
    collision = Colliders}

local function named(name)
    local s = stats[name]
    if not s then error("invalid name: " .. name) end
    if s and not profile.active(s) then profile.activate(s) end
    return timeline.named(name)
end

return setmetatable({
    timeline = tools.wrap(timeline.timeline),
    named = tools.wrap(named),
}, {__index = timeline})
