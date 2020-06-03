dofile "src/lua/path.lua"
local camera = require "nngn.lib.camera"
local entity = require "nngn.lib.entity"
local input = require "nngn.lib.input"
local texture = require "nngn.lib.texture"

require("nngn.lib.graphics").init()

local USE_COLLIDERS = false --true
local USE_COMPUTE = true

local N = math.tointeger(2 ^ 12)
local PARTICLE_SIZE = 4
local SIZE = 512
local BUFFER_SIZE = 4 * N
local TILE_SIZE = 32
local G = 9.8 * TILE_SIZE

nngn:set_compute(
    Compute.OPENCL_BACKEND,
    Compute.opencl_params{debug = true})
nngn.entities:set_max(N)
nngn.renderers:set_max_sprites(N)
nngn.graphics:resize_textures(3)
nngn.textures:set_max(3)
assert(nngn.textures:load(texture.NNGN))
nngn.colliders:set_max_colliders(N)

local LIMITS = nngn.compute:get_limits()
local LOCAL_SIZE = math.min(N, LIMITS[Compute.WORK_GROUP_SIZE + 1])

local center_x_buf = nngn.compute:create_buffer(
    Compute.READ_WRITE, Compute.FLOATV, N)
local center_y_buf = nngn.compute:create_buffer(
    Compute.READ_WRITE, Compute.FLOATV, N)
local radius_buf = nngn.compute:create_buffer(
    Compute.READ_WRITE, Compute.FLOATV, N)
local mass_buf = nngn.compute:create_buffer(
    Compute.READ_WRITE, Compute.FLOATV, N)
local vel_x_buf = nngn.compute:create_buffer(
    Compute.READ_WRITE, Compute.FLOATV, N)
local vel_y_buf = nngn.compute:create_buffer(
    Compute.READ_WRITE, Compute.FLOATV, N)
local force_x_buf = nngn.compute:create_buffer(
    Compute.READ_WRITE, Compute.FLOATV, N)
local force_y_buf = nngn.compute:create_buffer(
    Compute.READ_WRITE, Compute.FLOATV, N)

local function rnd_pos() return SIZE * (nngn.math:rand() - .5) end

local function init_pos(n, pos_b)
    local center_x = {}
    local center_y = {}
    for _ = 1, n do
        local x, y = rnd_pos(), rnd_pos()
        table.insert(center_x, x)
        table.insert(center_y, y)
    end
    local ret_v = Compute.create_vector(Compute.SIZEOF_FLOAT * BUFFER_SIZE)
    Compute.write_vector(ret_v, 0, {Compute.FLOATV, ret})
    assert(nngn.compute:write_buffer(
        center_x_buf, 0, N, Compute.FLOATV,
        Compute.to_bytes(Compute.FLOATV, center_x)))
    assert(nngn.compute:write_buffer(
        center_y_buf, 0, N, Compute.FLOATV,
        Compute.to_bytes(Compute.FLOATV, center_y)))
    assert(nngn.compute:fill_buffer(
        radius_buf, 0, N, Compute.FLOATV, {PARTICLE_SIZE / 2}))
    assert(nngn.compute:fill_buffer()mass_buf, 0, N, Compute.FLOATV, {1})
    assert(nngn.compute:fill_buffer(
        vel_x_buf, 0, N, Compute.FLOATV, {0}))
    assert(nngn.compute:fill_buffer(
        vel_y_buf, 0, N, Compute.FLOATV, {0}))
    assert(nngn.compute:fill_buffer(
        force_x_buf, 0, N, Compute.FLOATV, {0}))
    assert(nngn.compute:fill_buffer(
        force_y_buf, 0, N, Compute.FLOATV, {0}))
    return center_x, center_y
end

local function gen_entities(n)
    local ret = {}
    local t1 = dofile("src/lson/star.lua")
    t1.renderer.size = {PARTICLE_SIZE, PARTICLE_SIZE}
    table.insert(ret, entity.load(nil, nil, t1))
    local t = dofile("src/lson/circle8.lua")
    t.renderer.size = {PARTICLE_SIZE, PARTICLE_SIZE}
    for _ = 2, n do
        table.insert(ret, entity.load(nil, nil, t))
    end
    return ret
end

local grid = {size = SIZE / 2, n_cells = 16, max_idx = N}

function grid:init()
    local su = Compute.SIZEOF_UINT
    self.buf_count_size = self.n_cells ^ 2
    self.buf_count_bytes = su * self.buf_count_size
    self.buf_count = assert(
        nngn.compute:create_buffer(
            Compute.READ_WRITE, Compute.UINTV, self.buf_count_size))
    self.buf_size = self.max_idx * self.buf_count_size
    self.buf_bytes = su * self.buf_size
    self.buf = assert(
        nngn.compute:create_buffer(
            Compute.READ_WRITE, Compute.UINTV, self.buf_size))
    self.params_bytes = 3 * Compute.SIZEOF_FLOAT + 2 * Compute.SIZEOF_UINT
    self.params = Compute.create_vector(self.params_bytes)
    assert(Compute.write_vector(self.params, 0, {
        Compute.FLOAT, self.size,
        Compute.UINT, self.n_cells,
        Compute.UINT, grid.max_idx,
        Compute.FLOAT, G}))
end

function grid:update(prog, pos)
    assert(nngn.compute:fill_buffer(
        self.buf, 0, self.buf_size, Compute.UINTV, {0}))
    assert(nngn.compute:fill_buffer(
        self.buf_count, 0, self.buf_count_size, Compute.UINTV, {0}))
    local events = {}
    assert(nngn.compute:execute(
        prog, "grid", 0, {N}, {LOCAL_SIZE}, {
            Compute.DATA,
                {self.params_bytes, Compute.vector_data(self.params)},
            Compute.BUFFER, pos,
            Compute.BUFFER, self.buf,
            Compute.BUFFER, self.buf_count}, {}, events))
    return events
end

local function collision(prog, dt, grid, events)
    local wait = events
    events = {}
    assert(nngn.compute:execute(
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
    assert(nngn.compute:execute(
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
    assert(nngn.compute:release_events(events0))
    assert(nngn.compute:release_events(events1))
    assert(nngn.compute:read_buffer(pos, Compute.FLOATV, BUFFER_SIZE, pos_v))
    update_pos(
        entities,
        Compute.read_vector(
            nngn.compute:read_buffer(center_x_buf, Compute.FLOATV, N),
            0, N, Compute.FLOATV),
        Compute.read_vector(
            nngn.compute:read_buffer(center_y_buf, Compute.FLOATV, N),
            0, N, Compute.FLOATV))
end

local prog = assert(
    nngn.compute:create_program(
        io.open("demos/cl/particles.cl"):read("a"), "-Werror"))
local forces_b = assert(
    nngn.compute:create_buffer(
        Compute.READ_WRITE, Compute.FLOATV, BUFFER_SIZE))
local vel_b = assert(
    nngn.compute:create_buffer(
        Compute.READ_WRITE, Compute.FLOATV, BUFFER_SIZE))
local pos_b = assert(
    nngn.compute:create_buffer(
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
            m = -1e14, max_distance = 1024}}))
--    table.insert(gravity, entity.load(nil, nil, {
--        pos = {rnd_pos(), rnd_pos()},
--        renderer = star.renderer,
--        collider = {
--            type = Collider.GRAVITY, flags = Collider.SOLID,
--            m = 1e14, max_distance = 1024}}))
end

assert(nngn.compute:fill_buffer(forces_b, 0, BUFFER_SIZE, Compute.FLOATV, {0}))
assert(nngn.compute:fill_buffer(vel_b, 0, BUFFER_SIZE, Compute.FLOATV, {0}))

local x, y = init_pos(N, pos_b)
grid:init()
--grid:update(prog, pos_b)
local entities = gen_entities(N)
update_pos(entities, x, y)

local player = {0, 0}
for _, x in ipairs{"W", "A", "S", "D", " "} do
    input.input:remove(string.byte(x))
end
input.input:add(string.byte("A"), 0, function(_, press)
    if press then player[1] = player[1] - 1
    else player[1] = player[1] + 1 end
end)
input.input:add(string.byte("D"), 0, function(_, press)
    if press then player[1] = player[1] + 1
    else player[1] = player[1] - 1 end
end)
input.input:add(string.byte("S"), 0, function(_, press)
    if press then player[2] = player[2] - 1
    else player[2] = player[2] + 1 end
end)
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
        assert(nngn.compute:write_buffer(
            force_x_buf, 0, 1, Compute.FLOATV, input_v))
        assert(Compute.write_vector(input_v, 0, {
            Compute.FLOATV, {vel_scale * player[2]}}))
        assert(nngn.compute:write_buffer(
            force_y_buf, 0, 1, Compute.FLOATV, input_v))
        update(prog, grid, forces_b, vel_b, pos_b, entities)
    end
end

local hearbeat_key
input.input:add(string.byte(" "), Input.SEL_PRESS, function()
    if hearbeat_key then
        nngn.schedule:cancel(hearbeat_key)
        hearbeat_key = nil
        return
    end
    hearbeat_key = nngn.schedule:next(Schedule.HEARTBEAT, heartbeat)
end)
local explosion
nngn.input:register_binding(string.byte("E"), Input.BIND_PRESS, function()
    local timer = 250
    local collider = entities[1]:collider()
    load_entity(entities[1], nil, {
        collider = {
            type = Collider.GRAVITY, flags = Collider.SOLID,
            m = -1e14, max_distance = 32}})
    explosion = {
        heartbeat = nngn.schedule:next(Schedule.HEARTBEAT, function()
            timer = timer - nngn.timing:dt_ms()
            if timer > 0 then return end
            nngn.colliders:remove(entities[1]:collider())
            entities[1]:set_collider(collider)
            nngn.schedule:cancel(explosion.heartbeat)
            explosion = nil
        end)}
end)

nngn.grid:set_dimensions(32, 128)
nngn.colliders:set_max_collisions(2 * N)
nngn.colliders:set_resolve(false)
camera.reset(8)
