local tools <const> = require "nngn.lib.tools"

local ps = {}
local task

local FS = {}

local update
function update()
    task = tools.update(
        ps, task, update,
        function(k, v) k[2](v) end)
end

local function inspect(...)
    task = tools.init_w(
        ps, task, update,
        function() return io.popen("inspect", "w") end,
        function(k, v) k[1](v) end,
        ...)
end

return {
    FS = FS,
    inspect = inspect,
    named = function(name) return inspect(FS[name]) end,
}
