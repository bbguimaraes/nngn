dofile("src/lua/path.lua")
local nngn_camera = require "nngn.lib.camera"
local nngn_math = require "nngn.lib.math"

local IMG_WIDTH, IMG_HEIGHT = 512, 512
local IMG_BYTES = 4 * IMG_WIDTH * IMG_HEIGHT
local INTERNAL_TEX_BYTES = 3 * Compute.SIZEOF_FLOAT * IMG_WIDTH * IMG_HEIGHT
local BLOCK_SIZE, LOCAL_SIZE = 8, 16
local SAMPLES = 1024
local MAX_DEPTH = 64
local RND_STATE_SIZE = IMG_WIDTH * IMG_HEIGHT / BLOCK_SIZE ^ 2 * 4
local RND_STATE_BYTES = RND_STATE_SIZE * Compute.SIZEOF_UINT

local tracer = {
    i_samples = 0,
}

function tracer:init()
    self:init_camera()
    self:create_prog()
    self.local_size = nngn.compute:get_limits()[Compute.WORK_GROUP_SIZE + 1]
    self:init_tex()
    self:init_rnd()
    local n_spheres = self:init_spheres{
        0, 0, -1, 0.5,
        0, -100.5, -1, 100.0}
    self:init_conf(n_spheres)
end

function tracer:init_camera()
    local c = Camera.new()
    nngn_camera.set(c)
    nngn_camera.reset()
    c:set_perspective(true)
    c:set_max_vel(4)
    c:look_at(0, 0, -1, 0, 0, 0, 0, 1, 0)
    self.camera = c
end

function tracer:create_prog()
    local prog_args = {
        "-Werror",
        "-D BLOCK_SIZE=" .. BLOCK_SIZE .. "U",
        "-D PI=" .. math.pi .. "f",
        "-D TAU=" .. (2 * math.pi) .. "f",
        "-D T_MIN=0.001f",
        "-D T_MAX=INFINITY",
        "-D SKY_TOP=\"((float3){1, 1, 1})\"",
        "-D SKY_BOTTOM=\"((float3){0.5f, 0.7f, 1.0f})\""}
    self.prog = assert(
        nngn.compute:create_program(
            io.open("demos/cl/ray.cl"):read("a"),
            table.concat(prog_args, " ")))
end

function tracer:init_tex()
    self.tex = assert(nngn.compute:create_buffer(
        Compute.READ_WRITE, Compute.FLOATV, INTERNAL_TEX_BYTES))
    self.out = assert(nngn.compute:create_buffer(
        Compute.WRITE_ONLY, Compute.BYTEV, IMG_BYTES))
    nngn.compute:fill_buffer(
        self.tex, 0, INTERNAL_TEX_BYTES, Compute.FLOATV, {0})
end

function tracer:init_rnd()
    self.rnd_vector = Compute.create_vector(RND_STATE_BYTES)
    self.rnd_buffer = assert(nngn.compute:create_buffer(
        Compute.READ_ONLY, Compute.UINTV, RND_STATE_SIZE))
end

function tracer:init_spheres(t)
    local size = #t
    local n = size // 4
    assert(n * 4 == size)
    local bytes = 4 * Compute.SIZEOF_FLOAT * n
    self.sphere_buffer = assert(nngn.compute:create_buffer(
        Compute.READ_ONLY, Compute.FLOATV, bytes))
    nngn.compute:write_buffer(
        self.sphere_buffer, 0, size, Compute.FLOATV,
        Compute.to_bytes(Compute.FLOATV, t))
    return n
end

function tracer:init_conf(n_spheres)
    self.conf_bytes = 4 * Compute.SIZEOF_UINT + 3 * 4 * Compute.SIZEOF_FLOAT
    self.conf = Compute.create_vector(self.conf_bytes)
    Compute.write_vector(self.conf, 0, {
        Compute.UINT, IMG_WIDTH,
        Compute.UINT, IMG_HEIGHT,
        Compute.UINT, MAX_DEPTH,
        Compute.UINT, n_spheres,
        Compute.FLOATV, {
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 1, 0, 0}})
end

function tracer:update_camera()
    if not self.camera:update(nngn.timing) then return end
    self.i_samples = 0
    local p = nngn_math.vec3(self.camera:pos())
    local e = nngn_math.vec3(self.camera:eye())
    p = p + (p - e) * nngn_math.vec3(self.camera:zoom())
    Compute.write_vector(
        self.conf, 4 * Compute.SIZEOF_UINT,
        {Compute.FLOATV, {p[1], p[2], p[3], 0, e[1], e[2], e[3], 0}})
end

function tracer:update_rnd()
    nngn.math:fill_rnd_vec(0, RND_STATE_BYTES, self.rnd_vector)
    nngn.compute:write_buffer(
        self.rnd_buffer, 0, RND_STATE_SIZE, Compute.UINTV, self.rnd_vector)
end

function tracer:done() return self.i_samples == SAMPLES end

function tracer:trace()
    self:update_camera()
    self:update_rnd()
    local events = {}
    local args = {
        self.prog, nil, 0,
        {IMG_WIDTH / BLOCK_SIZE, IMG_HEIGHT / BLOCK_SIZE},
        {LOCAL_SIZE, LOCAL_SIZE},
        nil, nil, events}
    args[2] = "trace"
    args[6] = {
        Compute.DATA, {self.conf_bytes, Compute.vector_data(self.conf)},
        Compute.UINT, self.i_samples,
        Compute.BUFFER, self.rnd_buffer,
        Compute.BUFFER, self.sphere_buffer,
        Compute.BUFFER, self.tex}
    nngn.compute:execute(table.unpack(args))
    args[2] = "write_tex"
    args[4] = {IMG_HEIGHT}
    args[5] = {self.local_size}
    args[3] = Compute.BLOCKING
    args[6] = {
        Compute.UINT, IMG_WIDTH,
        Compute.BUFFER, self.tex,
        Compute.BUFFER, self.out}
    args[7] = events
    args[8] = nil
    nngn.compute:execute(table.unpack(args))
    nngn.compute:release_events(events)
    self.i_samples = self.i_samples + 1
end

require("nngn.lib.compute").init()
require("nngn.lib.graphics").init()
nngn.entities:set_max(1)
nngn.graphics:resize_textures(2)
nngn.textures:set_max(2)
nngn.renderers:set_max_sprites(1)
require("nngn.lib.input").install()
require("nngn.lib.camera").reset(2)

tracer:init()
local tex = nngn.textures:load_data("lua:ray", tracer.tex)
require("nngn.lib.entity").load(nil, nil, {
    renderer = {
        type = Renderer.SPRITE,  size = {512, 512}, tex = tex}})

local heartbeat = nngn.schedule:next(Schedule.HEARTBEAT, function()
    if tracer:done() then
        nngn.schedule:cancel(heartbeat)
    else
        tracer:trace()
        -- XXX
        local v = nngn.compute:read_buffer(tracer.out, Compute.BYTEV, IMG_BYTES)
        nngn.textures:update_data(tex, v)
    end
end)
