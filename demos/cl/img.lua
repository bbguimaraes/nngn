dofile "src/lua/path.lua"
local camera = require "nngn.lib.camera"
local entity = require "nngn.lib.entity"
require "src/lua/input"

local common = require "demos/cl/common"

require("nngn.lib.compute").init()
require("nngn.lib.graphics").init()
nngn.entities:set_max(5)
nngn.graphics:resize_textures(6)
nngn.textures:set_max(6)
nngn.renderers:set_max_sprites(5)

local LOCAL_SIZE = 16
local IMG_SIZE = 512
local IMG_PIXELS = IMG_SIZE * IMG_SIZE
local IMG_BYTES = 4 * IMG_PIXELS
local MAX_FILTER_SIZE =
    math.sqrt(
        nngn.compute:get_limits()[Compute.LOCAL_MEMORY + 1]
        / (4 * Compute.SIZEOF_FLOAT))
    - LOCAL_SIZE
local MAX_STD_DEV = MAX_FILTER_SIZE / 6

local function read_buf(b, v)
    nngn.compute:read_buffer(b, Compute.BYTEV, IMG_BYTES, v)
end

local function read_img(i, v)
    nngn.compute:read_image(i, IMG_SIZE, IMG_SIZE, Compute.BYTEV, v)
end

local prog = assert(
    nngn.compute:create_program(
        io.open("demos/cl/img.cl"):read("a"), "-Werror"))
local img_raw = Textures.read("img/petrov.png")
local img_buf = assert(
    nngn.compute:create_buffer(
        Compute.READ_ONLY, Compute.BYTEV, 0, img_raw))
local img = assert(
    nngn.compute:create_image(
        Compute.BYTEV, IMG_SIZE, IMG_SIZE, Compute.READ_ONLY, img_raw))
local sampler = assert(nngn.compute:create_sampler())

local rot = {
    a = 0,
    update = function(t, dt) t.a = t.a + dt / 10 end,
}

local br = {
    f = 0,
    update = function(t, dt) t.f = t.f + dt / 10 end,
}

local con = {
    filter_buf = assert(
        nngn.compute:create_buffer(
            Compute.READ_ONLY, Compute.FLOATV, MAX_FILTER_SIZE ^ 2)),
    update = function(t, dt)
        t:set_std_dev(math.min(MAX_STD_DEV, t.std_dev + dt / 32))
    end,
    set_std_dev = function(t, s)
        local w = math.tointeger(s * 6 // 1)
        if w % 2 == 0 then w = w + 1 end
        t.std_dev = s
        t.width = w
        t.radius = w // 2
        assert(nngn.compute:write_buffer(
            t.filter_buf, 0, w * w, Compute.FLOATV,
            Math.gaussian_filter_v(w, s)))
    end,
}

local fs = {{
    "rotate", {
        prog, img_buf, rot,
        assert(
            nngn.compute:create_buffer(
                Compute.WRITE_ONLY, Compute.BYTEV, IMG_BYTES)),
    },
    read_buf,
    function(events, prog, img_in, t, out)
        assert(nngn.compute:execute(
            prog, "rotate_img", 0,
            {IMG_BYTES}, {LOCAL_SIZE ^ 2}, {
                Compute.UINT, IMG_SIZE,
                Compute.UINT, IMG_SIZE,
                Compute.FLOAT, math.cos(t.a),
                Compute.FLOAT, math.sin(t.a),
                Compute.BUFFER, img_in,
                Compute.BUFFER, out}, {}, events))
    end,
}, {
    "brighten", {
        prog, img, sampler, br,
        assert(
            nngn.compute:create_image(
                Compute.BYTEV, IMG_SIZE, IMG_SIZE, Compute.WRITE_ONLY)),
    },
    read_img,
    function(events, prog, img_in, sampler, t, out)
        assert(nngn.compute:execute(
            prog, "brighten", 0,
            {IMG_SIZE, IMG_SIZE}, {LOCAL_SIZE, LOCAL_SIZE}, {
                Compute.FLOAT, math.abs(3 * math.sin(t.f)),
                Compute.SAMPLER, sampler,
                Compute.IMAGE, img_in,
                Compute.IMAGE, out}, {}, events))
    end,
}, {
    "convolution0", {
        prog, img, sampler, con,
        assert(
            nngn.compute:create_image(
                Compute.BYTEV, IMG_SIZE, IMG_SIZE, Compute.WRITE_ONLY)),
    },
    read_img,
    function(events, prog, img_in, sampler, t, img_out)
        assert(nngn.compute:execute(
            prog, "convolution0", 0,
            {IMG_SIZE, IMG_SIZE}, {LOCAL_SIZE, LOCAL_SIZE}, {
                Compute.UINT, IMG_SIZE,
                Compute.UINT, IMG_SIZE,
                Compute.BUFFER, t.filter_buf,
                Compute.UINT, t.radius,
                Compute.IMAGE, img_in,
                Compute.SAMPLER, sampler,
                Compute.IMAGE, img_out}, {}, events))
    end,
}, {
    "convolution1", {
        prog, img, sampler, con,
        assert(
            nngn.compute:create_image(
                Compute.BYTEV, IMG_SIZE, IMG_SIZE, Compute.WRITE_ONLY)),
    },
    read_img,
    function(events, prog, img_in, sampler, t, img_out)
        local size_pad = LOCAL_SIZE + 2 * t.radius
        local lm = Compute.SIZEOF_FLOAT * 4 * size_pad * size_pad
        assert(nngn.compute:execute(
            prog, "convolution1", 0,
            {IMG_SIZE, IMG_SIZE}, {LOCAL_SIZE, LOCAL_SIZE}, {
                Compute.UINT, IMG_SIZE,
                Compute.UINT, IMG_SIZE,
                Compute.BUFFER, t.filter_buf,
                Compute.UINT, t.radius,
                Compute.IMAGE, img_in,
                Compute.SAMPLER, sampler,
                Compute.LOCAL, lm,
                Compute.IMAGE, img_out}, {}, events))
    end,
}, {
    "convolution2", {
        prog, img, con,
        assert(
            nngn.compute:create_buffer(
                Compute.WRITE_ONLY, Compute.BYTEV, IMG_BYTES)),
    },
    read_buf,
    function(events, prog, buf_in, t, buf_out)
        local size_pad = LOCAL_SIZE + 2 * t.radius
        local lm = Compute.SIZEOF_FLOAT * 4 * size_pad * size_pad
        assert(nngn.compute:execute(
            prog, "convolution2", 0,
            {IMG_SIZE, IMG_SIZE}, {LOCAL_SIZE, LOCAL_SIZE}, {
                Compute.UINT, IMG_SIZE,
                Compute.UINT, IMG_SIZE,
                Compute.BUFFER, t.filter_buf,
                Compute.UINT, t.radius,
                Compute.BUFFER, buf_in,
                Compute.LOCAL, lm,
                Compute.BUFFER, buf_out}, {}, events))
    end,
}}

for i, p in ipairs({
    {-128, 0, 0}, {128, 0, 0},
    {-128, 128, 0}, {0, 128, 0}, {128, 128, 0},
}) do
    local t = fs[i]
    local tex = nngn.textures:load_data("lua:" .. t[1], img_raw)
    table.insert(t, tex)
    entity.load(nil, nil, {
        pos = p, renderer = {
            type = Renderer.SPRITE, tex = tex, size = {128, 128}}})
    nngn.textures:remove(tex)
end

con:set_std_dev(1)
for _, t in ipairs(fs) do
    local n, a, _, f = table.unpack(t)
    print(n)
    common.print_prof(
        common.avg_prof(5, function(e) f(e, table.unpack(a)) end))
end

local read_v = Compute.create_vector(IMG_BYTES)

nngn.schedule:next(Schedule.HEARTBEAT, function()
    local dt = nngn.timing:fdt_s()
    rot:update(dt)
    br:update(dt)
    con:update(dt)
    local events = {}
    for _, t in ipairs(fs) do
        local _, a, _, f = table.unpack(t)
        f(events, table.unpack(a))
    end
    assert(nngn.compute:wait(events))
    assert(nngn.compute:release_events(events))
    for i, t in ipairs(fs) do
        local _, a, r, _, t = table.unpack(t)
        r(a[#a], read_v)
        assert(nngn.textures:update_data(t, read_v))
    end
end)

collectgarbage("setpause", 100)
camera.reset(2)
