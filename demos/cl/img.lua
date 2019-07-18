dofile "src/lua/path.lua"
local camera <const> = require "nngn.lib.camera"
local entity <const> = require "nngn.lib.entity"
local input <const> = require("nngn.lib.input")
local img_common <const> = require "demos/cl/img_common"
local common <const> = require "demos/cl/common"

require("nngn.lib.compute").init()
require("nngn.lib.graphics").init()
require "src/lua/input"

local LIMITS <const> = nngn.compute:get_limits()
local LOCAL_SIZE <const> =
    math.floor(math.sqrt(LIMITS[Compute.WORK_GROUP_SIZE + 1]))
local N <const> = 1

img_common.set_limits(N)
local _, _, img <const> = img_common.init()
local prog <const> = assert(nngn.compute:create_program(
    io.open("demos/cl/img.cl"):read("a"), "-Werror"))

local F = 0
local fs <const> = {{
    kernel = "brighten",
    read = img_common.read_image,
    out = img_common.create_image(),
    f = function(t, events)
        assert(nngn.compute:execute(
            prog, t.kernel, 0,
            {img_common.IMG_SIZE, img_common.IMG_SIZE},
            {LOCAL_SIZE, LOCAL_SIZE}, {
                Compute.FLOAT, math.abs(3 * math.sin(F)),
                Compute.IMAGE, img,
                Compute.IMAGE, t.out,
            }, {}, events))
    end,
}}
assert(#fs == N)

local textures <const> = {}
img_common.init_images(fs, textures)
img_common.set_fn(function()
    nngn.schedule:next(Schedule.HEARTBEAT, function()
        F = F + nngn.timing:fdt_s() / 4
        img_common.update(fs, textures)
    end)
end)
camera.reset(1)
