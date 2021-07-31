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
    general = {
        float(
            "time scale (log₂)", -10000, 10000, 0, 1000,
            "nngn:timing():set_scale(2 ^ %1)"),
        bool("check collisions", true, "nngn:colliders():set_check(%1)"),
        bool(
            "resolve collisions", true,
            'require("nngn.lib.collision").set_resolve(%1)'),
    },
    limits = {
        int("textures", 2, 64, 16, 'require("nngn.lib.texture").set_max(%1)'),
        int("entities", 8, 1048576, 1048576, "nngn:entities():set_max(%1)"),
        int("animations", 8, 65536, 65536, "nngn:animations():set_max(%1)"),
        int(
            "sprites", 8, 65536, 65536,
            "nngn:renderers():set_max_sprites(%1)"),
        int(
            "translucent", 8, 128, 128,
            "nngn:renderers():set_max_translucent(%1)"),
        int(
            "cubes", 8, 65536, 65536,
            "nngn:renderers():set_max_cubes(%1)"),
        int(
            "voxels", 8, 65536, 65536,
            "nngn:renderers():set_max_voxels(%1)"),
        int(
            "text", 8, 65536, 65536,
            "nngn:renderers():set_max_text(%1)"),
        int("map", 8, 1048576, 1048576, "nngn:map():set_max(%1)"),
        int(
            "colliders", 8, 8192, 8192,
            'require("nngn.lib.collision").set_max_colliders(%1)'),
        int(
            "collisions", 8, 8192, 8192,
            "nngn:colliders():set_max_collisions(%1)"),
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
        bool("Z sprites", false, "nngn:renderers():set_zsprites(%1)"),
        bool("grid", false, "nngn:grid():set_enabled(%1)"),
        int(
            "grid spacing", 1, 1024, 32,
            "nngn:grid():set_dimensions(%1, nngn:grid():size())"),
        int(
            "grid size", 1, 1024, 64,
            "nngn:grid():set_dimensions(nngn:grid():spacing(), %1)"),
        bool("map", true, "nngn:map():set_enabled(%1)"),
        bool("lighting", false, "nngn:lighting():set_enabled(%1)"),
        bool("lighting Z sprites", false, "nngn:lighting():set_zsprites(%1)"),
        bool("update sun", true, "nngn:lighting():set_update_sun(%1)"),
        bool("shadows", false, "nngn:lighting():set_shadows_enabled(%1)"),
        float(
            "ambient light", 0, 100, 100, 100,
            "nngn:lighting():set_ambient_light(1, 1, 1, %1)"),
        float(
            "sun incidence", 0, 6283185, 5497787, 1e6,
            "nngn:lighting():sun():set_incidence(%1)"),
        int(
            "sun time", 0, 24 * 60 * 60 * 1000, 0,
            "nngn:lighting():sun():set_time_ms(%1)"),
        bool("textbox monospaced", false, "nngn:textbox():set_monospaced(%1)"),
        int("textbox speed", 0, 600, 16, "nngn:textbox():set_speed(%1)"),
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
        int(
            "shadow map size", 1, 4096, 1024,
            "nngn:graphics():set_shadow_map_size(%1, %1)"),
        int(
            "shadow cube size", 1, 4096, 512,
            "nngn:graphics():set_shadow_cube_size(%1, %1)"),
        float(
            "shadow map near", 0, 4096000, 1000, 1000,
            "nngn:lighting():set_shadow_map_near(%1, %1)"),
        float(
            "shadow map far", 0, 4096000, 1024000, 1000,
            "nngn:lighting():set_shadow_map_far(%1, %1)"),
        float(
            "shadow map proj. size", 0, 4096000, 128000, 1000,
            "nngn:lighting():set_shadow_map_proj_size(%1, %1)"),
    },
    post = {
        bool(
            "automatic exposure", false,
            "nngn:graphics():set_automatic_exposure(%1)"),
        float("HDR mix", 0, 100, 0, 100, "nngn:graphics():set_HDR_mix(%1)"),
        float(
            "exposure", 0, 1000, 100, 100,
            "nngn:graphics():set_exposure(%1)"),
        float(
            "bloom threshold", 0, 100, 100, 100,
            "nngn:graphics():set_bloom_threshold(%1)"),
        int(
            "bloom blur downscale", 1, 64, 0,
            "nngn:graphics():set_bloom_downscale(%1)"),
        float(
            "bloom blur size", 0, 1000, 100, 100,
            "nngn:graphics():set_bloom_blur_size(%1)"),
        int(
            "bloom blur passes", 0, 32, 10,
            "nngn:graphics():set_bloom_blur_passes(%1)"),
        float(
            "bloom amount", 0, 100, 0, 100,
            "nngn:graphics():set_bloom_amount(%1)"),
        int(
            "blur downscale", 1, 64, 2,
            "nngn:graphics():set_blur_downscale(%1)"),
        float(
            "blur size", 0, 1000, 100, 100,
            "nngn:graphics():set_blur_size(%1)"),
        int(
            "blur passes", 0, 32, 0,
            "nngn:graphics():set_blur_passes(%1)"),
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
