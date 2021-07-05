local os <const> = require "nngn.lib.os"
local tools <const> = require "nngn.lib.tools"

local ps = {}
local task

local function entry(s, title, text)
    return string.format("'%s:%s:%s\n'", s, title, text)
end

local function bool(title, init, text)
    return entry(string.format("b:%d", init and 1 or 0), title, text)
end

local function int(title, min, max, init, text)
    return entry(
        string.format("i:%d:%d:%d", min, max, init),
        title, text)
end

local function float(title, min, max, init, div, text)
    return entry(
        string.format("f:%d:%d:%d:%d", min, max, init, div),
        title, text)
end

local FS = {
    limits = {
        int("textures", 2, 64, 16, 'require("nngn.lib.texture").set_max(%1)'),
        int("entities", 8, 1048576, 1048576, "nngn:entities():set_max(%1)"),
        int(
            "sprites", 8, 65536, 65536,
            "nngn:renderers():set_max_sprites(%1)"),
        int(
            "cubes", 8, 65536, 65536,
            "nngn:renderers():set_max_cubes(%1)"),
        int(
            "voxels", 8, 65536, 65536,
            "nngn:renderers():set_max_voxels(%1)"),
    },
    camera = {
        bool("ortho./persp.", false, "nngn:camera():set_perspective(%1)"),
        bool("dash", false, "nngn:camera():set_dash(%1)"),
        float("dampening", 0, 2000, 500, 100, "nngn:camera():set_damp(%1)"),
        float("FOV Y", 0, 6283185, 1047198, 1e6, "nngn:camera():set_fov_y(%1)"),
        int("max vel.", 0, 1024, 64, "nngn:camera():set_max_vel(%1)"),
        float(
            "max rot vel.", 0, 16 * 6283185, 8 * 1e6, 1e6,
            "nngn:camera():set_max_rot_vel(%1)"),
        float(
            "max zoom vel.", 0, 10000, 1, 100,
            "nngn:camera():set_max_zoom_vel(%1)"),
        int(
            "eye x", -1024, 1024, 1,
            "do"
                .. " c = nngn:camera()"
                .. " v = {c:eye()};"
                .. " c:set_pos(%1, v[2], v[3])"
            .. " end"),
        int(
            "eye y", -1024, 1024, 1,
            "do"
                .. " c = nngn:camera()"
                .. " v = {c:eye()}"
                .. " c:set_pos(v[1], %1, v[3])"
            .. " end"),
        int(
            "eye z", -1024, 1024, 1,
            "do"
                .. " c = nngn:camera()"
                .. " v = {c:eye()}"
                .. " c:set_pos(v[1], v[2], %1)"
            .. " end"),
        int(
            "x acc", -1024, 1024, 1,
            "do"
                .. " c = nngn:camera()"
                .. " v = {c:acc()}"
                .. " c:set_acc(%1, v[2], v[3])"
            .. " end"),
        int(
            "y acc", -1024, 1024, 1,
            "do"
                .. " c = nngn:camera()"
                .. " v = {c:acc()}"
                .. " c:set_acc(v[1], %1, v[3])"
            .. " end"),
        int(
            "z acc", -1024, 1024, 1,
            "do"
                .. " c = nngn:camera()"
                .. " v = {c:acc()}"
                .. " c:set_acc(v[1], v[2], %1)"
            .. " end"),
        float(
            "x rot", 0, 6283185, 0, 1e6,
            "do"
                .. " c = nngn:camera()"
                .. " v = {c:rot()}"
                .. " c:set_rot(%1, v[2], v[3])"
            .. " end"),
        float(
            "y rot", 0, 6283185, 0, 1e6,
            "do"
                .. " c = nngn:camera()"
                .. " v = {c:rot()}"
                .. " c:set_rot(v[1], %1, v[3])"
            .. " end"),
        float(
            "z rot", 0, 6283185, 0, 1e6,
            "do"
                .. " c = nngn:camera()"
                .. " v = {c:rot()}"
                .. " c:set_rot(v[1], v[2], %1)"
            .. " end"),
        float(
            "zoom (log₂)", -1000, 1000, 0, 100,
            "nngn:camera():set_zoom(2 ^ %1)"),
    },
    render = {
        bool("perspective", false, "nngn:renderers():set_perspective(%1)"),
    },
    graphics = {
        bool(
            "cursor", true,
            "nngn:graphics():set_cursor_mode("
                .. "%1 and Graphics.CURSOR_MODE_NORMAL"
                .. " or Graphics.CURSOR_MODE_HIDDEN)"),
        int(
            "swap_interval", 0, 32, 1,
            "nngn:graphics():set_swap_interval(%1)"),
        int("swap chain length", 1, 16, 4, "nngn:graphics():set_n_frames(%1)"),
    },
}

local update
function update()
    task = tools.update(ps, task, update, function(k, v)
        while true do
            local s <const>, err = os.read_nonblock(v[1])
            local src
            if s then
                src, err = load(s)
            end
            if src then
                src()
            elseif err then
                io.stderr:write(debug.traceback(err), "\n")
            else
                return
            end
        end
    end)
end

local function configure(...)
    task = tools.init_r(
        ps, task, update,
        function(k)
            return io.popen("configure " .. table.concat(k, " "), "r")
        end,
        function(_, v) assert(Platform.set_non_blocking(v[1])) end,
        ...)
end

return {
    FS = FS,
    configure = configure,
    named = function(name) return configure(FS[name]) end,
}
