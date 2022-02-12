dofile "src/lua/path.lua"
local camera <const> = require "nngn.lib.camera"
local compute <const> = require "nngn.lib.compute"
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

local N <const> = 4
img_common.set_limits(N)

local LIMITS <const> = nngn:compute():get_limits()
local MAX_SIZE <const> = LIMITS[Compute.WORK_GROUP_SIZE + 1]
local MAX_SIZE2 <const> = math.sqrt(MAX_SIZE)
local LOCAL_SIZE <const> = common.local_size(img_common.IMG_SIZE, MAX_SIZE2)
local PIXEL_SIZE <const> = 4
local MIN_STD_DEV <const> = 0.6
local MAX_STD_DEV <const> = 128
local MIN_FILTER_WIDTH <const> = 3
local FILTER_MUL_FOR_STD_DEV <const> = 6

local GS <const> = {img_common.IMG_SIZE, img_common.IMG_SIZE}
local LS <const> = {LOCAL_SIZE, LOCAL_SIZE}

local function filter_size_round(s)
    local ret <const> = math.floor(s)
    return math.max(1, ret - (~ret & 1))
end

local MAX_LOCAL <const> = LIMITS[Compute.LOCAL_MEMORY + 1]
local MAX_FILTER_WIDTH_LOCAL <const> =
    filter_size_round(math.sqrt(MAX_LOCAL) / PIXEL_SIZE)
local MAX_STD_DEV_LOCAL <const> =
    MAX_FILTER_WIDTH_LOCAL / FILTER_MUL_FOR_STD_DEV
local FILTER_VEC_SIZE <const> =
    Compute.SIZEOF_FLOAT * MAX_STD_DEV * FILTER_MUL_FOR_STD_DEV
local FILTER_VEC_SIZE2 <const> =
    Compute.SIZEOF_FLOAT * (MAX_STD_DEV * FILTER_MUL_FOR_STD_DEV) ^ 2

local img_raw, img_buf, img, read_v <const> = img_common.init()
local prog <const> = assert(nngn:compute():create_program(
    io.open("demos/cl/img_conv.cl"):read("a"), "-Werror"))

local conv <const> = {
    std_dev = 0,
    width = 0,
    local_memory = 0,
    filter_buf = 0,
    filter2d_buf = 0,
    filter2d_local_buf = 0,
}

function conv:init()
    local sf <const> = Compute.SIZEOF_FLOAT
    local size <const> = MAX_STD_DEV * FILTER_MUL_FOR_STD_DEV
    local size2 <const> = size ^ 2
    self.counter = 0
    self.std_dev = 0.6
    self.filter_v = Compute.create_vector(sf * size)
    self.filter2d_v = Compute.create_vector(sf * size2)
    self.filter2d_local_v = Compute.create_vector(sf * size2)
    self.filter_buf = assert(nngn:compute():create_buffer(
        Compute.READ_ONLY, Compute.FLOATV, size))
    self.filter2d_buf = assert(nngn:compute():create_buffer(
        Compute.READ_ONLY, Compute.FLOATV, size2))
    self.filter2d_local_buf = assert(nngn:compute():create_buffer(
        Compute.READ_ONLY, Compute.FLOATV, size2))
    self:update(0)
end

function conv:update(t)
    local s, w <const> = self.filter_params(t, MAX_STD_DEV)
    local sl, wl <const> = self.filter_params(t, MAX_STD_DEV_LOCAL)
    assert(Compute.SIZEOF_FLOAT * w <= FILTER_VEC_SIZE)
    assert(Compute.SIZEOF_FLOAT * w * w <= FILTER_VEC_SIZE2)
    assert(Compute.SIZEOF_FLOAT * wl * wl <= FILTER_VEC_SIZE2)
    Math.gaussian_filter(w, s, self.filter_v)
    Math.gaussian_filter2d(w, w, s, self.filter2d_v)
    Math.gaussian_filter2d(wl, wl, sl, self.filter2d_local_v)
    assert(nngn:compute():write_buffer(
        self.filter_buf, 0, w, Compute.FLOATV, self.filter_v))
    assert(nngn:compute():write_buffer(
        self.filter2d_buf, 0, w * w, Compute.FLOATV, self.filter2d_v))
    assert(nngn:compute():write_buffer(
        self.filter2d_local_buf, 0, wl * wl, Compute.FLOATV,
        self.filter2d_local_v))
    self.std_dev = s
    self.width = w
    self.width_local = wl
    self.local_memory = (wl + LOCAL_SIZE) ^ 2 * PIXEL_SIZE
    assert(self.local_memory <= MAX_LOCAL)
end

function conv.filter_params(t, max)
    local s = math.max(MIN_STD_DEV, math.min(MAX_STD_DEV, t * max))
    local w = math.max(MIN_FILTER_WIDTH, math.floor(s * FILTER_MUL_FOR_STD_DEV))
    if w & 1 == 1 then return s, w end
    w = math.max(MIN_FILTER_WIDTH, filter_size_round(w))
    return w / FILTER_MUL_FOR_STD_DEV, w
end

local fs <const> = {{
--    kernel = "convolution0",
--    read = img_common.read_image,
--    out = img_common.create_image(),
--    f = function(t, events)
--        assert(nngn.compute:execute(prog, t.kernel, 0, GS, LS, {
--            Compute.BUFFER, conv.filter2d_buf,
--            Compute.UINT, conv.width // 2,
--            Compute.IMAGE, img,
--            Compute.IMAGE, t.out,
--        }, {}, events))
--    end,
--}, {
--    kernel = "convolution1",
--    read = img_common.read_image,
--    out = img_common.create_image(),
--    f = function(t, events)
--        assert(nngn.compute:execute(prog, t.kernel, 0, GS, LS, {
--            Compute.UINT, img_common.IMG_SIZE,
--            Compute.UINT, img_common.IMG_SIZE,
--            Compute.BUFFER, conv.filter2d_local_buf,
--            Compute.UINT, conv.width_local // 2,
--            Compute.IMAGE, img,
--            Compute.LOCAL, conv.local_memory,
--            Compute.IMAGE, t.out,
--        }, {}, events))
--    end,
--}, {
--    kernel = "convolution2",
--    read = img_common.read_buffer,
--    out = img_common.create_buffer(),
--    f = function(t, events)
--        assert(nngn.compute:execute(prog, t.kernel, 0, GS, LS, {
--            Compute.UINT, img_common.IMG_SIZE,
--            Compute.UINT, img_common.IMG_SIZE,
--            Compute.BUFFER, conv.filter2d_local_buf,
--            Compute.UINT, conv.width_local // 2,
--            Compute.BUFFER, img_buf,
--            Compute.LOCAL, conv.local_memory,
--            Compute.BUFFER, t.out,
--        }, {}, events))
--    end,
--}, {
    kernel = "blur",
    read = img_common.read_image,
    out = img_common.create_image(),
    img_tmp = img_common.create_image(Compute.READ_WRITE),
    f = function(t, events)
        assert((compute:kernel():new(prog, t.kernel, 0, gs, ls, {
                Compute.UINT, conv.width // 2,
                Compute.UINT, 0,
                Compute.BUFFER, conv.filter_buf,
                Compute.IMAGE, img,
                Compute.IMAGE, t.img_tmp,
            }) | compute:kernel():new(prog, t.kernel, 0, gs, ls, {
                Compute.UINT, 0,
                Compute.UINT, conv.width // 2,
                Compute.BUFFER, conv.filter_buf,
                Compute.IMAGE, t.img_tmp,
                Compute.IMAGE, t.out,
            })
        ):execute({}, events))
    end,
}}
--assert(#fs == N)

conv:init()
conv:update(1 / MAX_STD_DEV)
local textures <const> = {}
img_common.init_images(fs, textures)

function demo_start()
    conv:init()
    local t = 0
    local counter = 0
    task = nngn:schedule():next(Schedule.HEARTBEAT, function()
        counter = math.min(counter, math.pi)
        if counter == math.pi then
            img_common.read_buffer(img_buf, read_v)
            nngn:textures():update_data(textures[1], read_v)
            return
        else
            conv:update(math.abs(math.sin(counter)))
        end
        counter = counter + nngn:timing():fdt_s()
        img_common.update(fs, textures)
    end)
end
img_common.set_fn(demo_start)
camera.reset(1)

require("nngn.lib.font").load()
nngn.renderers:set_max_text(1024)
require("nngn.lib.textbox").update("image processing", "convolution")
