local PLOT = {}

local function write(f, prefix, ...)
    for _, x in ipairs{...} do f:write(prefix, x, "\n") end
end

local function write_names(f, ...) write(f, "g l ", ...) end
local function write_data(f, ...) write(f, "d l ", ...) end

local FS = {
    fps = {
        function(f)
            f:write("s 2\n")
            write_names(f, "last_dt", "avg", "sec_count", "sec_last")
        end,
        function(f)
            local d = nngn.fps:dump()
            write_data(f, d.last_dt, d.avg, d.sec_count, d.sec_last)
        end},
    lua = {
        function(f)
            f:write("s 1\n")
            write_names(f, "lua_mem_kb", "lua_refs")
        end,
        function(f)
            write_data(f, collectgarbage("count"), #debug.getregistry())
        end}}

function PLOT.heartbeat() PLOT:update() end

function PLOT:update()
    local p = self.p
    if nngn.lua.ferror(p) ~= 0 then return self:finish() end
    p:write("f ", nngn.timing:now_ms(), "\n")
    for _, t in ipairs(self.fs) do t[2](p) end
    p:flush()
end

function PLOT:quit()
    self.p:write("q\n")
    self.p:flush()
    self.p:close()
    self:finish()
end

function PLOT:finish()
    nngn.schedule:cancel(self.k)
    self.p = nil
end

function PLOT:plot(...)
    if self.p then return self:quit() end
    local fs = {...}
    local p = io.popen("plot", "w")
    p:setvbuf("line")
    for _, t in ipairs(fs) do t[1](p) end
    p:flush()
    self.fs = fs
    self.p = p
    self.k = nngn.schedule:next(Schedule.HEARTBEAT, PLOT.heartbeat)
end

local function plot(...) return PLOT:plot(...) end

local function func(name, f)
    return plot{
        function(...) write_names(..., name) end,
        function(...) write_data(..., f()) end}
end

local function named(name) return plot(FS[name]) end
local function eval(code) return func(code, load("return " .. code)) end

return {
    FS = FS,
    plot = plot,
    func = func,
    named = named,
    eval = eval,
    write_names = write_names,
    write_data = write_data,
}
