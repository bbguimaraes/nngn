local tools <const> = require "nngn.lib.tools"

local ps = {}
local task

local function write(p, prefix, v)
    p:write(prefix)
    for _, x in ipairs(v) do
        p:write(" ", x)
    end
    p:write("\n")
end

local FS = {
    lua = {
        function(p)
            write(p, "g", {
                "string", "table", "fn", "userdata", "thread", "others",
            })
        end,
        function(p)
            local info <const> = state.alloc_info()
            local t <const> = {}
            for i = 1, #info, 2 do
                table.insert(t, 0)
                table.insert(t, 0)
                table.insert(t, 0)
                table.insert(t, info[i])
            end
            write(p, "d", t)
        end,
    },
    profile = {
        function(p) write(p, "g", Profile.stats_names()) end,
        function(p) write(p, "d", Profile.stats_as_timeline()) end,
    },
}

local update
function update()
    task = tools.update(
        ps, task, update,
        function(k, v) k[2](v[1]) end)
end

local function timeline(...)
    task = tools.init_w(
        ps, task, update,
        function() return io.popen("cat", "w") end,
        function(k, v) k[1](v[1]) end,
        ...)
end

return {
    FS = FS,
    timeline = timeline,
    named = function(name) return timeline(FS[name]) end,
}
