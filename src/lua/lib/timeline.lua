local TIMELINE = {}

local function write(p, prefix, v)
    p:write(prefix)
    for _, x in ipairs(v) do p:write(" ", x) end
    p:write("\n")
end

local FS = {
    profile = {
        function(p) write(p, "g", Profile.stats_names()) end,
        function(p) write(p, "d", Profile.stats_as_timeline()) end},
    collision = {
        function(p) write(p, "g", Colliders.stats_names()) end,
        function(p) write(p, "d", Colliders.stats()) end}}

function TIMELINE.heartbeat() TIMELINE:update() end

function TIMELINE:update()
    local p = self.p
    if nngn.lua.ferror(p) ~= 0 then return self:finish() end
    self.f(p)
    p:flush()
end

function TIMELINE:quit()
    local p = self.p
    p:write("q\n")
    p:flush()
    p:close()
    self:finish()
end

function TIMELINE:finish()
    nngn.schedule:cancel(self.k)
    self.p = nil
end

function TIMELINE:timeline(f)
    if self.p then return self:quit() end
    local p = io.popen("timeline", "w")
    p:setvbuf("line")
    f[1](p)
    p:flush()
    self.f = f[2]
    self.p = p
    self.k = nngn.schedule:next(Schedule.HEARTBEAT, TIMELINE.heartbeat)
end

local function timeline(...) return TIMELINE:timeline(...) end
local function named(name) return timeline(FS[name]) end

return {
    FS = FS,
    timeline = timeline,
    named = named,
}
