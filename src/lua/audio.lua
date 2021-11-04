local audio = require "nngn.lib.audio"
local tools = require "nngn.lib.tools"

return setmetatable({
    audio = tools.wrap(audio.audio),
}, {__index = audio})
