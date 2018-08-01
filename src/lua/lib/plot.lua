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
        end},
    camera = {
        function(f)
            f:write("s 3\n")
            write_names(f,
                "pos_x", "vel_x", "acc_x",
                "pos_y", "vel_y", "acc_y",
                "pos_z", "vel_z", "acc_z",
                "rot_x", "rot_vel_x", "rot_acc_x",
                "rot_y", "rot_vel_y", "rot_acc_y",
                "rot_z", "rot_vel_z", "rot_acc_z",
                "zoom", "zoom_vel", "zoom_acc")
        end,
        function(f)
            local c <const> = nngn.camera
            local p, v, a <const> = {c:pos()}, {c:vel()}, {c:acc()}
            local r, rv, ra <const> = {c:rot()}, {c:rot_vel()}, {c:rot_acc()}
            write_data(f,
                p[1], v[1], a[1],
                p[2], v[2], a[2],
                p[3], v[3], a[3],
                r[1], rv[1], ra[1],
                r[2], rv[2], ra[2],
                r[3], rv[3], ra[3],
                c:zoom(), c:zoom_vel(), c:zoom_acc())
        end,
    },
    graphics = {
        function(f)
            f:write("s 2\n")
            write_names(f,
                "staging.req.n_allocations",
                "staging.req.total_memory_kb",
                "staging.n_allocations",
                "staging.n_reused",
                "staging.n_free",
                "staging.total_memory_kb",
                "buffers.n_writes",
                "buffers.total_writes_kb")
        end,
        function(f)
            local stats = nngn.graphics:stats()
            local stg, buf = stats.staging, stats.buffers
            local kb = 0x1p-10
            write_data(f,
                stg.req_n_allocations,
                stg.req_total_memory * kb,
                stg.n_allocations,
                stg.n_reused,
                stg.n_free,
                stg.total_memory * kb,
                buf.n_writes,
                buf.total_writes_bytes * kb)
        end},
    coll = {
        function(f) write_names(f, "coll") end,
        function(f) write_data(f, nngn.colliders:n_collisions()) end}}

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
