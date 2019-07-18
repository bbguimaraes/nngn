dofile "src/lua/path.lua"
local camera = require "nngn.lib.camera"
local entity = require "nngn.lib.entity"
require "src/lua/input"

local common = require "demos/cl/common"

nngn:set_compute(Compute.OPENCL_BACKEND, Compute.opencl_params{debug = true})
require("nngn.lib.graphics").init()
nngn:entities():set_max(7)
nngn:graphics():resize_textures(8)
nngn:textures():set_max(8)
nngn:renderers():set_max_sprites(7)

local IMG_SIZE <const> = 512
local IMG_PIXELS <const> = IMG_SIZE * IMG_SIZE
local IMG_BYTES <const> = 4 * IMG_PIXELS

local HIST_SIZE <const> = 4 * 256
local HIST_BYTES <const> = 4 * HIST_SIZE

local LIMITS <const> = nngn:compute():get_limits()
local COMPUTE_UNITS <const> = LIMITS[Compute.COMPUTE_UNITS + 1]
local GLOBAL_SIZE <const> = common.round_down_pow2(COMPUTE_UNITS)
local LOCAL_SIZE <const> =
    math.min(IMG_SIZE // 2, LIMITS[Compute.WORK_GROUP_SIZE + 1])
local LOCAL_SIZE_SQRT <const> = math.sqrt(LOCAL_SIZE)

local function exec_hist_to_tex(prog, hist, id, buf_in, v)
    local out = assert(
        nngn:compute():create_image(
            Compute.BYTEV, IMG_SIZE, IMG_SIZE, Compute.WRITE_ONLY))
    local masks = {4, 2, 1, 7}
    for channel = 0, 3 do
        local max = 0
        for i = 1, 256 do max = math.max(max, hist[256 * channel + i]) end
        nngn:compute():execute(
            prog, "hist_to_tex", Compute.BLOCKING,
            {IMG_SIZE // 2}, {LOCAL_SIZE}, {
                Compute.UINT, channel,
                Compute.UINT, max,
                Compute.BYTE, masks[channel + 1],
                Compute.BUFFER, buf_in,
                Compute.IMAGE, out})
    end
    assert(nngn:compute():read_image(out, IMG_SIZE, IMG_SIZE, Compute.BYTEV, v))
    nngn:textures():update_data(id, v)
    nngn:compute():release_image(out)
end

local prog <const> = assert(
    nngn:compute():create_program(
        io.open("demos/cl/hist.cl"):read("a"), "-Werror"))
local img_raw <const> = nngn:textures().read("img/petrov.png")
local img_buf <const> = assert(
    nngn:compute():create_buffer(Compute.READ_ONLY, 0, 0, img_raw))
local img <const> = assert(
    nngn:compute():create_image(
        Compute.BYTEV, IMG_SIZE, IMG_SIZE, Compute.READ_ONLY, img_raw))
local sampler <const> = assert(nngn:compute():create_sampler())

local fs = {{
    "hist0", function(f, events, out)
        assert(nngn:compute():execute(
            prog, f, Compute.BLOCKING,
            {IMG_SIZE, IMG_SIZE}, {LOCAL_SIZE_SQRT, LOCAL_SIZE_SQRT}, {
                Compute.IMAGE, img,
                Compute.SAMPLER, sampler,
                Compute.BUFFER, out,
            }, {}, events))
    end,
}, {
    "hist1", function(f, events, out)
        assert(nngn:compute():execute(
            prog, f, Compute.BLOCKING,
            {IMG_SIZE, IMG_SIZE}, {LOCAL_SIZE_SQRT, LOCAL_SIZE_SQRT}, {
                Compute.UINT, IMG_SIZE,
                Compute.BUFFER, img_buf,
                Compute.BUFFER, out,
            }, {}, events))
    end,
}, {
    "hist2", function(f, events, out)
        assert(nngn:compute():execute(
            prog, f, Compute.BLOCKING,
            {GLOBAL_SIZE * LOCAL_SIZE}, {LOCAL_SIZE}, {
                Compute.UINT, IMG_PIXELS,
                Compute.BUFFER, img_buf,
                Compute.BUFFER, out,
            }, {}, events))
    end,
}, {
    "hist3", function(f, events, out)
        assert(nngn:compute():execute(
            prog, f, Compute.BLOCKING,
            {GLOBAL_SIZE * LOCAL_SIZE}, {LOCAL_SIZE}, {
                Compute.UINT, IMG_PIXELS,
                Compute.BUFFER, img_buf,
                Compute.BUFFER, out,
            }, {}, events))
    end,
}, {
    "hist4", function(f, events, out)
        assert(nngn:compute():execute(
            prog, f, Compute.BLOCKING,
            {GLOBAL_SIZE * LOCAL_SIZE}, {LOCAL_SIZE}, {
                Compute.UINT, IMG_PIXELS,
                Compute.LOCAL, HIST_BYTES,
                Compute.BUFFER, img_buf,
                Compute.BUFFER, out,
            }, {}, events))
    end,
}, {
    "hist5", function(f, events, out)
        assert(nngn:compute():execute(
            prog, f, Compute.BLOCKING,
            {GLOBAL_SIZE * LOCAL_SIZE}, {LOCAL_SIZE}, {
                Compute.UINT, IMG_PIXELS,
                Compute.LOCAL, 16 * HIST_BYTES,
                Compute.BUFFER, img_buf,
                Compute.BUFFER, out,
            }, {}, events))
    end,
}}

local out = assert(
    nngn:compute():create_buffer(
        Compute.WRITE_ONLY, Compute.UINTV, HIST_SIZE))
local v = Compute.create_vector(IMG_BYTES)
local check = (function()
    local name, f = table.unpack(fs[1])
    f(name, nil, out)
    assert(nngn:compute():read_buffer(out, Compute.UINTV, HIST_SIZE, v))
    local cmp = Compute.read_vector(v, 0, HIST_SIZE, Compute.UINTV)
    return function(t) return common.vec_eq(t, cmp) end
end)()
for i, p in ipairs({
    {64, 192, 0}, {320, 192, 0},
    {64, 320, 0}, {320, 320, 0},
    {64, 448, 0}, {320, 448, 0},
}) do
    local name, f = table.unpack(fs[i])
    print(name)
    local tex = nngn:textures():load_data(string.format("lua:%s", name))
    entity.load(nil, nil, {
        pos = p,
        renderer = {type = Renderer.SPRITE, tex = tex, size = {128, 128}},
    })
    nngn:textures():remove(tex)
    common.print_prof(common.avg_prof(5, function(e) f(name, e, out) end))
    nngn:compute():fill_buffer(out, 0, HIST_SIZE, Compute.UINTV, {0})
    f(name, nil, out)
    assert(nngn:compute():read_buffer(out, Compute.UINTV, HIST_SIZE, v))
    local out_t = Compute.read_vector(v, 0, HIST_SIZE, Compute.UINTV)
    check(out_t)
    exec_hist_to_tex(prog, out_t, tex, out, v)
end
do
    local tex <const> = nngn:textures():load("img/petrov.png")
    entity.load(nil, nil, {
        pos = {-192, 320, 0},
        renderer = {type = Renderer.SPRITE, tex = tex, size = {384, 384}},
    })
end
nngn:compute():release_buffer(out)
nngn:grid():set_dimensions(128, 64)
nngn:grid():set_enabled(true)
camera.reset(1)
nngn:camera():set_pos(0, 320, 0)
function demo_start() end
