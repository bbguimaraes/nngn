dofile "src/lua/path.lua"
local camera = require "nngn.lib.camera"
local entity = require "nngn.lib.entity"
local input = require "nngn.lib.input"
local texture = require "nngn.lib.texture"

require("nngn.lib.graphics").init()

local N <const> = math.tointeger(2 ^ 12)
local PARTICLE_SIZE <const> = 4
local SIZE <const> = 512
local BUFFER_SIZE <const> = 4 * N
local TILE_SIZE <const> = 32
local GRID_SIZE <const> = SIZE / 2
local GRID_CELLS <const> = 16
local G <const> = 9.8 * TILE_SIZE
local KERNEL <const> = "collision_by_particle" -- "collision_by_grid"

nngn:set_compute(
    Compute.OPENCL_BACKEND,
    Compute.opencl_params{
        preferred_device = Compute.DEVICE_TYPE_GPU,
        version = {3, 0},
    })
nngn:entities():set_max(N)
nngn:renderers():set_max_sprites(N)
nngn:graphics():resize_textures(3)
nngn:textures():set_max(3)
assert(nngn:textures():load(texture.NNGN))

local LIMITS <const> = nngn:compute():get_limits()
local LOCAL_SIZE <const> = LIMITS[Compute.WORK_GROUP_SIZE + 1]

local grid <const> = {kernel = 0}

function grid:init(prog, pos)
    local cs <const> = GRID_CELLS * GRID_CELLS
    self.count_buffer_size = cs
    self.count_buffer = assert(nngn:compute():create_buffer(
        Compute.READ_WRITE, Compute.UINTV, cs))
    local s <const> = N * cs
    self.buffer_size = s
    self.buffer = assert(nngn:compute():create_buffer(
        Compute.READ_WRITE, Compute.UINTV, s))
    local pb <const> = 4 * Compute.SIZEOF_FLOAT + 2 * Compute.SIZEOF_UINT
    self.params_bytes = pb
    local params_v = Compute.create_vector(pb)
    assert(Compute.write_vector(params_v, 0, {
        Compute.UINT, N,
        Compute.UINT, GRID_CELLS,
        Compute.FLOAT, GRID_SIZE,
        Compute.FLOAT, PARTICLE_SIZE / 2,
        Compute.FLOAT, 0.15,
        Compute.FLOAT, G,
    }))
    self.params = assert(nngn:compute():create_buffer(
        Compute.READ_ONLY, Compute.BYTEV, 0, params_v))
    self.kernel = assert(nngn:compute():create_kernel(prog, "grid", {
        Compute.BUFFER, self.params,
        Compute.BUFFER, pos,
        Compute.BUFFER, self.buffer,
        Compute.BUFFER, self.count_buffer,
    }))
end

function grid:update()
    local events = {}
    assert(nngn:compute():fill_buffer(
        self.buffer, 0, self.buffer_size, Compute.UINTV, {0}, {}, events))
    assert(nngn:compute():fill_buffer(
        self.count_buffer, 0, self.count_buffer_size, Compute.UINTV, {0},
        {}, events))
    assert(nngn:compute():execute_kernel(
        self.kernel, 0, {N}, {LOCAL_SIZE}, events, events))
    return events
end

local function collision(prog, dt, grid, pos, vel, forces, wait)
    local events = {}
    assert(nngn:compute():execute(
        prog, KERNEL, 0, {N}, {LOCAL_SIZE}, {
            Compute.BUFFER, grid.params,
            Compute.FLOAT, dt,
            Compute.BUFFER, grid.buffer,
            Compute.BUFFER, grid.count_buffer,
            Compute.BUFFER, pos,
            Compute.BUFFER, vel,
            Compute.BUFFER, forces,
        }, wait, events))
    return events
end

local function integrate(prog, dt, forces, vel, pos, wait)
    local events = {}
    assert(nngn:compute():execute(
        prog, "integrate", 0, {N}, {LOCAL_SIZE}, {
            Compute.FLOAT, dt,
            Compute.BUFFER, forces,
            Compute.BUFFER, vel,
            Compute.BUFFER, pos,
        }, wait, events))
    return events
end

local function update(dt, prog, grid, forces, vel, pos_v, pos, entities)
    local events = grid:update()
    collision(prog, dt, grid, pos, vel, forces, events)
    integrate(prog, dt, forces, vel, pos, events)
    assert(nngn:compute():wait(events))
    assert(nngn:compute():release_events(events))
    assert(nngn:compute():read_buffer(pos, Compute.FLOATV, BUFFER_SIZE, pos_v))
    nngn:entities():set_pos4(pos_v)
end

local prog <const> = assert(nngn:compute():create_program(
    io.open("demos/cl/particles.cl"):read("a"), "-Werror"))
local forces_b <const> = assert(nngn:compute():create_buffer(
    Compute.READ_WRITE, Compute.FLOATV, BUFFER_SIZE))
local vel_b <const> = assert(nngn:compute():create_buffer(
    Compute.READ_WRITE, Compute.FLOATV, BUFFER_SIZE))
local pos_b <const> = assert(nngn:compute():create_buffer(
    Compute.READ_WRITE, Compute.FLOATV, BUFFER_SIZE))

assert(nngn:compute():fill_buffer(
    forces_b, 0, BUFFER_SIZE, Compute.FLOATV, {0}))
assert(nngn:compute():fill_buffer(
    vel_b, 0, BUFFER_SIZE, Compute.FLOATV, {0}))

local pos_v, pos <const> = (function()
    local pos <const> = nngn:math():rand_tablef(N * 2)
    local ret = {}
    for i = 0, N - 1 do
        table.insert(ret, SIZE * (pos[2 * i + 1] - 0.5))
        table.insert(ret, SIZE * (pos[2 * i + 2] - 0.5))
        table.insert(ret, 0)
        table.insert(ret, 0)
    end
    local ret_v = Compute.create_vector(Compute.SIZEOF_FLOAT * BUFFER_SIZE)
    assert(Compute.write_vector(ret_v, 0, {Compute.FLOATV, ret}))
    assert(nngn:compute():write_buffer(
        pos_b, 0, BUFFER_SIZE, Compute.FLOATV, ret_v))
    return ret_v, ret
end)()
local entities <const> = (function()
    local ret = {}
    local t = dofile("src/lson/circle8.lua")
--    local t1 = dofile("src/lson/star.lua")
--    t1.renderer.size = {PARTICLE_SIZE, PARTICLE_SIZE}
    t.renderer.size = {PARTICLE_SIZE, PARTICLE_SIZE}
--    table.insert(ret, entity.load(nil, nil, t1))
    table.insert(ret, entity.load(nil, nil, t))
--    local t = dofile("src/lson/circle8.lua")
    t.renderer.size = {PARTICLE_SIZE, PARTICLE_SIZE}
    for _ = 2, N do
        table.insert(ret, entity.load(nil, nil, t))
    end
    return ret
end)()
nngn:entities():set_pos4(pos_v)
grid:init(prog, pos_b)
grid:update()

local player <const> = {0, 0}
input.input:remove(string.byte("A"))
input.input:add(string.byte("A"), 0, function(_, press)
    if press then player[1] = player[1] - 1
    else player[1] = player[1] + 1 end
end)
input.input:remove(string.byte("D"))
input.input:add(string.byte("D"), 0, function(_, press)
    if press then player[1] = player[1] + 1
    else player[1] = player[1] - 1 end
end)
input.input:remove(string.byte("S"))
input.input:add(string.byte("S"), 0, function(_, press)
    if press then player[2] = player[2] - 1
    else player[2] = player[2] + 1 end
end)
input.input:remove(string.byte("W"))
input.input:add(string.byte("W"), 0, function(_, press)
    if press then player[2] = player[2] + 1
    else player[2] = player[2] - 1 end
end)
input.install()

local input_v = Compute.create_vector(Compute.FLOATV, 2)
local function heartbeat()
    local vel_scale = 16 * TILE_SIZE
    local dt = nngn:timing():fdt_s()
    local n = 2
    for i = 0, n - 1 do
        assert(Compute.write_vector(input_v, 0, {
            Compute.FLOATV, {vel_scale * player[1], vel_scale * player[2]}}))
        assert(nngn:compute():write_buffer(forces_b, 0, 2, Compute.FLOATV, input_v))
        update(dt / n, prog, grid, forces_b, vel_b, pos_v, pos_b, entities)
    end
end

local hearbeat_key
function demo_start()
    hearbeat_key = nngn:schedule():next(Schedule.HEARTBEAT, heartbeat)
end
input.input:remove(string.byte(" "))
input.input:add(string.byte(" "), Input.SEL_PRESS, function()
    if hearbeat_key then
        nngn:schedule():cancel(hearbeat_key)
        hearbeat_key = nil
    else
        demo_start()
    end
end)
camera.reset(1)
