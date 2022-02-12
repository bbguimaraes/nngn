dofile "src/lua/path.lua"
local camera <const> = require "nngn.lib.camera"
local input <const> = require("nngn.lib.input")
local common <const> = require "demos.cl.common"
local img_common <const> = require "demos.cl.img_common"
require "src/lua/input"

nngn:set_compute(
    Compute.OPENCL_BACKEND,
    Compute.opencl_params{
        preferred_device = Compute.DEVICE_TYPE_GPU,
        debug = true,
    })
require("nngn.lib.graphics").init()
local N <const> = 3
img_common.set_limits(N)

local MAX_LOCAL <const> =
    nngn:compute():get_limits()[Compute.WORK_GROUP_SIZE + 1]
local LOCAL_SIZE_BYTES <const> =
    common.local_size(img_common.IMG_BYTES, MAX_LOCAL)
local LOCAL_SIZE2 <const> =
    common.local_size(img_common.IMG_SIZE, math.sqrt(MAX_LOCAL))
local LOCAL_SIZE <const> =
    common.local_size(img_common.IMG_SIZE, math.sqrt(MAX_LOCAL))

local img_raw, img_buf, img, read_v <const> = img_common.init()
local prog <const> = assert(nngn:compute():create_program(
    io.open("demos/cl/img_rot.cl"):read("a"), "-Werror"))

local ANGLE = 0
local fs <const> = {{
--    kernel = "rotate0",
--    read = img_common.read_buffer,
--    out = img_common.create_buffer(),
--    f = function(t, events)
--        assert(nngn.compute:execute(
--            prog, t.kernel, 0, {img_common.IMG_BYTES}, {LOCAL_SIZE_BYTES}, {
--                Compute.UINT, img_common.IMG_SIZE,
--                Compute.UINT, img_common.IMG_SIZE,
--                Compute.FLOAT, math.cos(ANGLE),
--                Compute.FLOAT, math.sin(ANGLE),
--                Compute.BUFFER, img_buf,
--                Compute.BUFFER, t.out,
--            }, {}, events))
--    end,
--}, {
--    kernel = "rotate1",
--    read = img_common.read_buffer,
--    out = img_common.create_buffer(),
--    f = function(t, events)
--        assert(nngn.compute:execute(
--            prog, t.kernel, 0, {img_common.IMG_SIZE ^ 2}, {LOCAL_SIZE2}, {
--                Compute.UINT, img_common.IMG_SIZE,
--                Compute.UINT, img_common.IMG_SIZE,
--                Compute.FLOAT, math.cos(ANGLE),
--                Compute.FLOAT, math.sin(ANGLE),
--                Compute.BUFFER, img_buf,
--                Compute.BUFFER, t.out,
--            }, {}, events))
--    end,
--}, {
    kernel = "rotate2",
    read = img_common.read_image,
    out = img_common.create_image(),
    f = function(t, events)
        assert(nngn:compute():execute(
            prog, t.kernel, 0,
            {img_common.IMG_SIZE, img_common.IMG_SIZE},
            {LOCAL_SIZE, LOCAL_SIZE}, {
                Compute.UINT, img_common.IMG_SIZE,
                Compute.UINT, img_common.IMG_SIZE,
                Compute.FLOAT, math.cos(ANGLE),
                Compute.FLOAT, math.sin(ANGLE),
                Compute.IMAGE, img,
                Compute.IMAGE, t.out,
            }, {}, events))
    end,
}}
--assert(#fs == N)

local textures <const> = {}
img_common.init_images(fs, textures)
img_common.set_fn(function()
    task = nngn:schedule():next(Schedule.HEARTBEAT, function()
        ANGLE = ANGLE + nngn:timing():fdt_s()
        ANGLE = math.min(ANGLE, 2 * math.pi)
        img_common.update(fs, textures)
    end)
end
img_common.set_fn(demo_start)
camera.reset(1)

require("nngn.lib.font").load()
nngn.renderers:set_max_text(1024)
require("nngn.lib.textbox").update("image processing", "rotation")
