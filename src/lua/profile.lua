local profile = require "nngn.lib.profile"

local function wrap(f)
    return function(c, ...)
        print(c, profile.active(c))
        if profile.active(c) then return f(c, ...) end
        profile.activate(c)
        local args = {...}
        nngn.schedule:next(0, function()
            nngn.schedule:next(0, function() f(c, table.unpack(args)) end)
        end)
    end
end

return setmetatable({
    dump_text = wrap(profile.dump_text),
    dump_tui = wrap(profile.dump_tui),
}, {__index = profile})
