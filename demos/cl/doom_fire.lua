dofile "src/lua/path.lua"
local entity = require "nngn.lib.entity"

require("nngn.lib.compute").init()
require("nngn.lib.graphics").init()
require("nngn.lib.input").install()

nngn.graphics:resize_textures(2)
nngn.textures:set_max(2)
nngn.entities:set_max(1)
nngn.renderers:set_max_sprites(1)

local IMG_SIZE <const> = 512
local IMG_PIXELS <const> = IMG_SIZE * IMG_SIZE
local IMG_BYTES <const> = 4 * IMG_PIXELS
local LOCAL_SIZE <const> = math.min(
    math.tointeger(IMG_SIZE / 2),
    nngn.compute:get_limits()[Compute.WORK_GROUP_SIZE + 1])
local MAX_PALETTE <const> = 37
assert(math.tointeger(math.log(IMG_SIZE, 2)))
assert(IMG_SIZE % LOCAL_SIZE == 0)

local buf = assert(nngn.compute:create_buffer(
    Compute.READ_WRITE, Compute.BYTEV, IMG_PIXELS))
local img = assert(nngn.compute:create_image(
    Compute.BYTEV, IMG_SIZE, IMG_SIZE, Compute.WRITE_ONLY))
local tex_v = Compute.create_vector(IMG_BYTES)
local rnd = (function()
    local size = 4 * Compute.SIZEOF_UINT * LOCAL_SIZE
    local v = Compute.create_vector(size)
    nngn.math:fill_rnd_vec(0, size, v)
    return assert(nngn.compute:create_buffer(
        Compute.READ_WRITE, Compute.BYTEV, 0, v))
end)()
local kernel = nngn.compute:create_kernel(
    assert(
        nngn.compute:create_program(
            io.open("demos/cl/rnd.cl"):read("a")
                .. io.open("demos/cl/doom_fire.cl"):read("a"),
            "-Werror -DMAX_PALETTE=" .. MAX_PALETTE)),
    "update", {
        Compute.UINT, IMG_SIZE,
        Compute.UINT, IMG_SIZE,
        Compute.BUFFER, rnd,
        Compute.BUFFER, buf,
        Compute.IMAGE, img,
    })

assert(nngn.compute:fill_image(
    img, IMG_SIZE, IMG_SIZE, Compute.BYTEV, {0, 0, 0, 0}))
assert(nngn.compute:fill_buffer(
    buf, 0, IMG_PIXELS - IMG_SIZE, Compute.BYTEV, {0}))
assert(nngn.compute:fill_buffer(
    buf, IMG_PIXELS - IMG_SIZE, IMG_SIZE, Compute.BYTEV, {MAX_PALETTE - 1}))

local tex = nngn.textures:load_data("lua:doom_fire", tex_v)
entity.load(nil, nil, {
    renderer = {
        type = Renderer.SPRITE,
        tex = tex, size = {IMG_SIZE, IMG_SIZE},
    },
})

input = require("nngn.lib.input")
input.input:remove(string.byte(" "))
input.input:add(string.byte(" "), Input.SEL_PRESS, function()
    local task = nngn.schedule:next(Schedule.HEARTBEAT, function()
        assert(nngn.compute:execute_kernel(
            kernel, Compute.BLOCKING, {IMG_SIZE}, {LOCAL_SIZE}))
        assert(nngn.compute:read_image(
            img, IMG_SIZE, IMG_SIZE, Compute.BYTEV, tex_v))
        nngn.textures:update_data(tex, tex_v)
    end)
    nngn.schedule:in_ms(0, 3000, function()
        assert(nngn.compute:fill_buffer(
            buf, IMG_PIXELS - IMG_SIZE, IMG_SIZE, Compute.BYTEV, {0}))
    end)
    nngn.schedule:in_ms(0, 6000, function()
        nngn.schedule:cancel(task)
    end)
end)

nngn.camera:set_pos(0, -192, 0)
nngn.camera:set_zoom(1.5)
