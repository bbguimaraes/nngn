dofile "src/lua/path.lua"
local camera = require "nngn.lib.camera"
local entity = require "nngn.lib.entity"
local input = require "nngn.lib.input"
local texture = require "nngn.lib.texture"

require("nngn.lib.graphics").init()

local USE_COLLIDERS = true
local USE_COMPUTE = true

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
    Compute.opencl_params{debug = true})
nngn:entities():set_max(N + 6)
nngn:renderers():set_max_sprites(N + 5)
nngn:graphics():resize_textures(3)
nngn:textures():set_max(3)
assert(nngn:textures():load(texture.NNGN))
nngn:colliders():set_max_collisions(2 * N)
nngn:colliders():set_max_colliders(N)
nngn:colliders():set_max_collisions(2 * N)
nngn:colliders():set_resolve(false)
nngn:grid():set_dimensions(32, 128)
camera.reset(8)

local LIMITS <const> = nngn:compute():get_limits()
local LOCAL_SIZE <const> = math.min(N, LIMITS[Compute.WORK_GROUP_SIZE + 1])

local center_x_buf = nngn:compute():create_buffer(
    Compute.READ_WRITE, Compute.FLOATV, N)
local center_y_buf = nngn:compute():create_buffer(
    Compute.READ_WRITE, Compute.FLOATV, N)
local radius_buf = nngn:compute():create_buffer(
    Compute.READ_WRITE, Compute.FLOATV, N)
local mass_buf = nngn:compute():create_buffer(
    Compute.READ_WRITE, Compute.FLOATV, N)
local vel_x_buf = nngn:compute():create_buffer(
    Compute.READ_WRITE, Compute.FLOATV, N)
local vel_y_buf = nngn:compute():create_buffer(
    Compute.READ_WRITE, Compute.FLOATV, N)
local force_x_buf = nngn:compute():create_buffer(
    Compute.READ_WRITE, Compute.FLOATV, N)
local force_y_buf = nngn:compute():create_buffer(
    Compute.READ_WRITE, Compute.FLOATV, N)

local grid <const> = {kernel = 0}

local function init_pos(n, pos_b)
    local center_x <const> = {}
    local center_y <const> = {}
    for _ = 1, n do
        local x, y = rnd_pos(), rnd_pos()
        table.insert(center_x, x)
        table.insert(center_y, y)
    end
    local ret_v = Compute.create_vector(Compute.SIZEOF_FLOAT * BUFFER_SIZE)
    Compute.write_vector(ret_v, 0, {Compute.FLOATV, ret})
    assert(nngn:compute():write_buffer(
        center_x_buf, 0, N, Compute.FLOATV,
        Compute.to_bytes(Compute.FLOATV, center_x)))
    assert(nngn:compute():write_buffer(
        center_y_buf, 0, N, Compute.FLOATV,
        Compute.to_bytes(Compute.FLOATV, center_y)))
    assert(nngn:compute():fill_buffer(
        radius_buf, 0, N, Compute.FLOATV, {PARTICLE_SIZE / 2}))
    assert(nngn:compute():fill_buffer(mass_buf, 0, N, Compute.FLOATV, {1}))
    assert(nngn:compute():fill_buffer(
        vel_x_buf, 0, N, Compute.FLOATV, {0}))
    assert(nngn:compute():fill_buffer(
        vel_y_buf, 0, N, Compute.FLOATV, {0}))
    assert(nngn:compute():fill_buffer(
        force_x_buf, 0, N, Compute.FLOATV, {0}))
    assert(nngn:compute():fill_buffer(
        force_y_buf, 0, N, Compute.FLOATV, {0}))
    return center_x, center_y
end

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

local function collision(prog, dt, grid, events)
    local wait = events
    events = {}
    assert(nngn:compute():execute(
        -- XXX
--        prog, "collision0", Compute.BLOCKING, {N / 4}, {LOCAL_SIZE / 4}, {
        prog, "collision1", Compute.BLOCKING, {N}, {LOCAL_SIZE}, {
            Compute.DATA,
                {grid.params_bytes, Compute.vector_data(grid.params)},
            Compute.UINT, N,
            Compute.FLOAT, dt,
            Compute.BUFFER, grid.buf,
            Compute.BUFFER, grid.buf_count,
            Compute.BUFFER, center_x_buf,
            Compute.BUFFER, center_y_buf,
            Compute.BUFFER, radius_buf,
            Compute.BUFFER, mass_buf,
            Compute.BUFFER, vel_x_buf,
            Compute.BUFFER, vel_y_buf,
            Compute.BUFFER, force_x_buf,
            Compute.BUFFER, force_y_buf}, wait, events))
    return events
end

local function integrate(prog, dt, forces, vel, pos, events)
    assert(nngn:compute():execute(
        -- XXX
        prog, "integrate", Compute.BLOCKING, {N / 4}, {LOCAL_SIZE / 4}, {
            Compute.FLOAT, dt,
            Compute.BUFFER, force_x_buf,
            Compute.BUFFER, force_y_buf,
            Compute.BUFFER, vel_x_buf,
            Compute.BUFFER, vel_y_buf,
            Compute.BUFFER, center_x_buf,
            Compute.BUFFER, center_y_buf}, events))
end

local function update_pos(e, x, y)
    local set_pos = Entity.set_pos
    for i = 1, #e do
        local i4 = 4 * i
        set_pos(e[i], x[i], y[i], 0)
    end
end

local function update(prog, grid, forces, vel, pos_v, pos, entities)
    local dt = nngn.timing:fdt_s()
    local events0 = grid:update(prog, pos)
    local events1 = collision(prog, dt, grid)
    integrate(prog, dt, forces, vel, pos)
    assert(nngn:compute():release_events(events0))
    assert(nngn:compute():release_events(events1))
    assert(nngn:compute():read_buffer(pos, Compute.FLOATV, BUFFER_SIZE, pos_v))
    update_pos(
        entities,
        Compute.read_vector(
            nngn:compute():read_buffer(center_x_buf, Compute.FLOATV, N),
            0, N, Compute.FLOATV),
        Compute.read_vector(
            nngn:compute():read_buffer(center_y_buf, Compute.FLOATV, N),
            0, N, Compute.FLOATV))
end

local prog <const> = assert(nngn:compute():create_program(
    io.open("demos/cl/particles.cl"):read("a"), "-Werror"))
local forces_b <const> = assert(nngn:compute():create_buffer(
    Compute.READ_WRITE, Compute.FLOATV, BUFFER_SIZE))
local vel_b <const> = assert(nngn:compute():create_buffer(
    Compute.READ_WRITE, Compute.FLOATV, BUFFER_SIZE))
local pos_b <const> = assert(nngn:compute():create_buffer(
    Compute.READ_WRITE, Compute.FLOATV, BUFFER_SIZE))

local gravity = {}
if USE_COLLIDERS then
    for _, t in ipairs({
        {{SIZE / -2, 0, 0}, {1, SIZE}, { 1, 0, 0}},
        {{SIZE /  2, 0, 0}, {1, SIZE}, { -1, 0, 0}},
        {{0, SIZE / -2, 0}, {SIZE, 1}, {0,  1, 0}},
        {{0, SIZE /  2, 0}, {SIZE, 1}, {0, -1, 0}},
    }) do
        entity.load(nil, nil, {
            pos = t[1],
            renderer = {
                type = Renderer.SPRITE, tex = 1,
                size = t[2], scale = {512, 512}, coords = {1, 0}},
            collider = {
                type = Collider.PLANE, flags = Collider.SOLID,
                m = 1/0, n = t[3]}})
    end
    local star = dofile("src/lson/star.lua")
    table.insert(gravity, entity.load(nil, nil, {
        renderer = star.renderer,
        collider = {
            type = Collider.GRAVITY, flags = Collider.SOLID,
            m = -1e14, max_distance = 1024,
        },
    }))
--    table.insert(gravity, entity.load(nil, nil, {
--        pos = {rnd_pos(), rnd_pos()},
--        renderer = star.renderer,
--        collider = {
--            type = Collider.GRAVITY, flags = Collider.SOLID,
--            m = 1e14, max_distance = 1024,
--        },
--    }))
end

assert(nngn:compute():fill_buffer(
    forces_b, 0, BUFFER_SIZE, Compute.FLOATV, {0}))
assert(nngn:compute():fill_buffer(
    vel_b, 0, BUFFER_SIZE, Compute.FLOATV, {0}))

local x, y = init_pos(N, pos_b)
grid:init()
--grid:update(prog, pos_b)
local entities = gen_entities(N)
update_pos(entities, x, y)

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
    local vel_scale = 20 * 32 -- 1024
    if USE_COLLIDERS then
        local pa = {entities[1]:acc()}
        pa[1] = pa[1] + vel_scale * player[1]
        pa[2] = pa[2] + vel_scale * player[2]
        entities[1]:set_acc(table.unpack(pa))
    elseif USE_COMPUTE then
        assert(Compute.write_vector(input_v, 0, {
            Compute.FLOATV, {vel_scale * player[1]}}))
        assert(nngn:compute():write_buffer(
            force_x_buf, 0, 1, Compute.FLOATV, input_v))
        assert(Compute:write_vector(input_v, 0, {
            Compute.FLOATV, {vel_scale * player[2]}}))
        assert(nngn:compute():write_buffer(
            force_y_buf, 0, 1, Compute.FLOATV, input_v))
        update(prog, grid, forces_b, vel_b, pos_b, entities)
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
local explosion
input.input:remove(string.byte("E"))
input.input:add(string.byte("E"), Input.SEL_PRESS, function()
    local timer = 250
    local collider = entities[1]:collider()
    entity.load(entities[1], nil, {
        collider = {
            type = Collider.GRAVITY, flags = Collider.SOLID,
            m = -1e14, max_distance = 32,
        },
    })
    explosion = {
        heartbeat = nngn:schedule():next(Schedule.HEARTBEAT, function()
            timer = timer - nngn:timing():dt_ms()
            if 0 < timer then
                return
            end
            nngn:colliders():remove(entities[1]:collider())
            entities[1]:set_collider(collider)
            nngn:schedule():cancel(explosion.heartbeat)
            explosion = nil
        end),
    }
end)
