local camera <const> = require "nngn.lib.camera"
local tools <const> = require "nngn.lib.tools"

local ps = {}
local task

local function write(f, prefix, ...)
    for _, x in ipairs{...} do f:write(prefix, x, "\n") end
end

local function write_names(f, ...) write(f, "g l ", ...) end
local function write_data(f, ...) write(f, "d l ", ...) end

local FS = {
    fps = {
        function(f)
            f:write("s 4\n")
            write_names(f, "last_dt", "avg", "sec_count", "sec_last")
        end,
        function(f)
            local d = nngn:fps():dump()
            write_data(f, d.last_dt, d.avg, d.sec_count, d.sec_last)
        end,
    },
    lua = {
        function(f)
            f:write("s 4\n")
            write_names(f,
                "mem_kb", "refs",
                "alloc_total_n", "alloc_total_kb",
                "alloc_string_n", "alloc_string_kb",
                "alloc_table_n", "alloc_table_kb",
                "alloc_fn_n", "alloc_fn_kb",
                "alloc_userdata_n", "alloc_userdata_kb",
                "alloc_thread_n", "alloc_thread_kb",
                "alloc_others_n", "alloc_others_kb")
        end,
        function(f)
            local t <const> = state.alloc_info()
            table.move(t, 1, #t, 3)
            t[1], t[2] = 0, 0
            for i = 4, #t, 2 do
                local v <const> = t[i] / 0x1p10
                t[i] = v
                t[1] = t[1] + t[i - 1]
                t[2] = t[2] + v
            end
            write_data(f,
                collectgarbage("count"),
                #debug.getregistry(),
                table.unpack(t))
        end,
    },
    render = {
        function(f)
            f:write("s 4\n")
            write_names(f, "sprites", "translucents", "cubes", "voxels")
        end,
        function(f)
            local r <const> = nngn:renderers()
            write_data(f,
                r:n_sprites(), r:n_translucent(), r:n_cubes(), r:n_voxels())
        end,
    },
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
            local c <const> = camera.get()
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
                "textures",
                "",
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
            local stats = nngn:graphics():stats()
            local stg, buf = stats.staging, stats.buffers
            local kb = 0x1p-10
            write_data(f,
                nngn:textures():n(), 0,
                stg.req_n_allocations,
                stg.req_total_memory * kb,
                stg.n_allocations,
                stg.n_reused,
                stg.n_free,
                stg.total_memory * kb,
                buf.n_writes,
                buf.total_writes_bytes * kb)
        end,
    },
    coll = {
        function(f)
            f:write("s 5\n")
            write_names(f,
                "aabbs", "bbs", "spheres", "planes", "gravity",
                "coll")
        end,
        function(f)
            local c <const> = nngn:colliders()
            write_data(f,
                c:n_aabbs(), c:n_bbs(), c:n_spheres(), c:n_planes(),
                c:n_gravity(),
                c:n_collisions())
        end,
    },
}

local update
function update()
    local now <const> = nngn:timing():now_ms()
    task = tools.update(ps, task, update, function(k, v)
        local p <const> = v[1]
        p:write("f ", now, "\n")
        k[2](p)
    end)
end

local function plot(...)
    task = tools.init_w(
        ps, task, update,
        function() return io.popen("plot", "w") end,
        function(k, v) k[1](v[1]) end,
        ...)
end

local function func(name, f)
    return plot{
        function(...) write_names(..., name) end,
        function(...) write_data(..., f()) end,
    }
end

local function eval(code) return func(code, load("return " .. code)) end

return {
    FS = FS,
    plot = plot,
    named = function(name) return plot(FS[name]) end,
    func = func,
    eval = eval,
    write_names = write_names,
    write_data = write_data,
}
