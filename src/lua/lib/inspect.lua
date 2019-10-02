local tools <const> = require "nngn.lib.tools"

local ps = {}
local task

local FS = {
    textures = {
        function() end,
        function(t)
            local p <const>, last_gen <const> = table.unpack(t)
            local gen <const> = nngn.textures:generation()
            if gen == last_gen then
                return
            end
           t[2] = gen
            p:write("c\nl Textures\nl generation ", gen, '\n')
            for _, x in ipairs(nngn.textures:dump()) do
                p:write("l ", x[2], " ", x[1], "\n")
            end
        end,
    },
}

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
