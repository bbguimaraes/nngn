local INSPECT = nil

local FS = {
    textures = {
        function() return {last_gen = nngn.textures:generation()} end,
        function(p, d)
            local gen = nngn.textures:generation()
            if gen == d.last_gen then return end
            d.last_gen = gen
            p:write("c\nl Textures\nl generation ", gen, '\n')
            for _, x in ipairs(nngn.textures:dump()) do
                p:write("l ", x[2], " ", x[1], "\n")
            end
        end,
    },
}

local function inspect(init_f, update_f)
    if INSPECT ~= nil then
        nngn.schedule:cancel(INSPECT.k)
        INSPECT.p:write("q\n")
        INSPECT.p:flush()
        INSPECT.p:close()
        INSPECT = nil
        return
    end
    local p = io.popen("inspect", "w")
    local d = init_f()
    INSPECT = {
        p = p,
        k = nngn.schedule:next(Schedule.HEARTBEAT, function()
            update_f(p, d)
            p:flush()
        end)
    }
end

local function named(name) return inspect(FS[name]) end

return {
    FS = FS,
    inspect = inspect,
    named = named,
}
