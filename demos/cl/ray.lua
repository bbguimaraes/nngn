dofile("src/lua/path.lua")
local nngn_camera = require "nngn.lib.camera"
local nngn_math = require "nngn.lib.math"

local vec3 = nngn_math.vec3

local rand = (function()
    local m = nngn.math
    local f = Math.rand
    return function() return f(m) end
end)()

local IMG_WIDTH, IMG_HEIGHT = 512, 512
local IMG_BYTES = 4 * IMG_WIDTH * IMG_HEIGHT
local INTERNAL_TEX_BYTES = 3 * Compute.SIZEOF_FLOAT * IMG_WIDTH * IMG_HEIGHT
local BLOCK_SIZE, LOCAL_SIZE = 8, 16
local SAMPLES = 1024 * 1024
local MAX_DEPTH = 64
local RND_STATE_SIZE = IMG_WIDTH * IMG_HEIGHT / BLOCK_SIZE ^ 2 * 4
local RND_STATE_BYTES = RND_STATE_SIZE * Compute.SIZEOF_UINT
local MATERIAL_TYPE_BITS = 2
local MATERIAL_TYPE_LAMBERTIAN = 0
local MATERIAL_TYPE_METAL = 1
local MATERIAL_TYPE_DIELECTRIC = 2
local MATERIAL_METAL = 1 << (32 - MATERIAL_TYPE_BITS)
local MATERIAL_DIELECTRIC =
    MATERIAL_TYPE_DIELECTRIC << (32 - MATERIAL_TYPE_BITS)

local tracer = {
    i_samples = 0,
}

function tracer:init()
    self:init_camera()
    self:create_prog()
    self.local_size = nngn.compute:get_limits()[Compute.WORK_GROUP_SIZE + 1]
    self:init_tex()
    self:init_rnd()
    local spheres = {}
    table.insert(spheres, {
        pos ={0, -1000, 0},
        radius = 1000,
        material = {type = MATERIAL_TYPE_LAMBERTIAN, albedo = {0.5, 0.5, 0.5}}})
    local spread = 2
    for a = -spread, spread - 1 do
        for b = -spread, spread - 1 do
            local center = vec3(a + 0.9 * rand(), 0.2, b + 0.9 * rand())
            if center:len_sq() <= 0.9 * 0.9 then
                goto continue
            end
            local sphere = {pos = center, radius = 0.2}
            local mat = rand()
            if mat < 0.8 then
                mat = {
                    type = MATERIAL_TYPE_LAMBERTIAN,
                    albedo = nngn_math.vec3_rand(rand)
                        * nngn_math.vec3_rand(rand)}
            elseif mat < 0.95 then
                mat = {
                    type = MATERIAL_TYPE_METAL,
                    albedo = nngn_math.vec3_rand(rand) / vec3(2) + vec3(0.5),
                    fuzz = rand(0, 0.5)}
            else
                mat = {type = MATERIAL_TYPE_DIELECTRIC, n1_n0 = 1.5}
            end
            sphere.material = mat
            table.insert(spheres, sphere)
            ::continue::
        end
    end
    table.insert(spheres, {
        pos = {0, 1, 0},
        radius = 1,
        material = {type = MATERIAL_TYPE_DIELECTRIC, n1_n0 = 1.5}})
    table.insert(spheres, {
        pos = {-4, 1, 0},
        radius = 1,
        material = {type = MATERIAL_TYPE_LAMBERTIAN, albedo = {0.4, 0.2, 0.1}}})
    table.insert(spheres, {
        pos = {4, 1, 0},
        radius = 1,
        material = {
            type = MATERIAL_TYPE_METAL, albedo = {0.7, 0.6, 0.5}, fuzz = 0}})
    local n_spheres = self:init_spheres(spheres)
    self:init_conf(n_spheres)
end

function tracer:init_camera()
    local c = Camera.new()
    nngn_camera.set(c)
    nngn_camera.reset()
    c:set_perspective(true)
    c:set_max_vel(4)
    c:look_at(0, 0, 0, 13, 2, 3, 0, 1, 0)
    c:set_zoom(10)
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
        "-D MATERIAL_TYPE_LAMBERTIAN=" .. MATERIAL_TYPE_LAMBERTIAN .. "U",
        "-D MATERIAL_TYPE_METAL=" .. MATERIAL_TYPE_METAL .. "U",
        "-D MATERIAL_TYPE_DIELECTRIC=" .. MATERIAL_TYPE_DIELECTRIC .. "U",
        "-D MATERIAL_TYPE_BITS=" .. MATERIAL_TYPE_BITS .. "U",
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
    local SF = Compute.SIZEOF_FLOAT
    local SF3, SF4 = 3 * SF, 4 * SF
    local type_lamb, type_metal, type_diel =
        MATERIAL_TYPE_LAMBERTIAN, MATERIAL_TYPE_METAL, MATERIAL_TYPE_DIELECTRIC
    local n = #t
    local n_materials = {[type_lamb] = 0, [type_metal] = 0, [type_diel] = 0}
    for _, s in ipairs(t) do
        local type = s.material.type
        local count = n_materials[type]
        if count == nil then error("invalid material: " .. mat.type) end
        n_materials[type] = count + 1
    end
    local sphere_bytes = 2 * SF4 * n
    local lamb_bytes = SF4 * n_materials[type_lamb]
    local metal_bytes = SF4 * n_materials[type_metal]
    local diel_bytes = SF * n_materials[type_diel]
    local vector = Compute.create_vector(sphere_bytes)
    local lamb_vector = Compute.create_vector(lamb_bytes)
    local metal_vector = Compute.create_vector(metal_bytes)
    local diel_vector = Compute.create_vector(diel_bytes)
    Compute.fill_vector(vector, 0, sphere_bytes, Compute.BYTEV, {0})
    Compute.fill_vector(lamb_vector, 0, lamb_bytes, Compute.BYTEV, {0})
    Compute.fill_vector(metal_vector, 0, metal_bytes, Compute.BYTEV, {0})
    Compute.fill_vector(diel_vector, 0, diel_bytes, Compute.BYTEV, {0})
    local sphere_off, lamb_off, metal_off, diel_off = 0, 0, 0, 0
    local i_materials = {[type_lamb] = 0, [type_metal] = 0, [type_diel] = 0}
    for _, s in ipairs(t) do
        Compute.write_vector(vector, sphere_off, {
            Compute.FLOATV, s.pos,
            Compute.FLOAT, s.radius})
        sphere_off = sphere_off + SF4
        local mat = s.material
        local i_mat = i_materials[mat.type]
        if mat.type == type_lamb then
            Compute.write_vector(vector, sphere_off, {Compute.UINT, i_mat})
            Compute.write_vector(
                lamb_vector, lamb_off, {Compute.FLOATV, mat.albedo})
            lamb_off = lamb_off + SF4
            i_materials[type_lamb] = i_mat + 1
        elseif mat.type == type_metal then
            Compute.write_vector(
                vector, sphere_off, {Compute.UINT, MATERIAL_METAL | i_mat})
            Compute.write_vector(metal_vector, metal_off, {
                Compute.FLOATV, mat.albedo,
                Compute.FLOAT, mat.fuzz})
            metal_off = metal_off + SF4
            i_materials[type_metal] = i_mat + 1
        elseif mat.type == type_diel then
            Compute.write_vector(
                vector, sphere_off, {Compute.UINT, MATERIAL_DIELECTRIC | i_mat})
            Compute.write_vector(
                diel_vector, diel_off, {Compute.FLOAT, mat.n1_n0})
            diel_off = diel_off + SF
            i_materials[type_diel] = i_mat + 1
        else
            error("invalid material: " .. mat.type)
        end
        sphere_off = sphere_off + SF4
        assert(sphere_off <= sphere_bytes)
        assert(lamb_off <= lamb_bytes)
        assert(metal_off <= metal_bytes)
        assert(diel_off <= diel_bytes)
        assert(i_materials[type_lamb] <= n_materials[type_lamb])
        assert(i_materials[type_metal] <= n_materials[type_metal])
        assert(i_materials[type_diel] <= n_materials[type_diel])
    end
    assert(sphere_off == sphere_bytes)
    assert(lamb_off == lamb_bytes)
    assert(metal_off == metal_bytes)
    assert(diel_off == diel_bytes)
    assert(i_materials[type_lamb] == n_materials[type_lamb])
    assert(i_materials[type_metal] == n_materials[type_metal])
    assert(i_materials[type_diel] == n_materials[type_diel])
    local buffer = assert(nngn.compute:create_buffer(
        Compute.READ_ONLY, Compute.FLOATV, sphere_bytes))
    nngn.compute:write_buffer(
        buffer, 0, sphere_bytes, Compute.BYTEV, vector)
    local lamb_buffer = assert(nngn.compute:create_buffer(
        Compute.READ_ONLY, Compute.FLOATV, lamb_bytes))
    nngn.compute:write_buffer(
        lamb_buffer, 0, lamb_bytes, Compute.FLOATV, lamb_vector)
    local metal_buffer = assert(nngn.compute:create_buffer(
        Compute.READ_ONLY, Compute.FLOATV, metal_bytes))
    nngn.compute:write_buffer(
        metal_buffer, 0, metal_bytes, Compute.FLOATV, metal_vector)
    local diel_buffer = assert(nngn.compute:create_buffer(
        Compute.READ_ONLY, Compute.FLOATV, diel_bytes))
    nngn.compute:write_buffer(
        diel_buffer, 0, diel_bytes, Compute.FLOATV, diel_vector)
    self.lambertian_buffer = lamb_buffer
    self.metal_buffer = metal_buffer
    self.dielectric_buffer = diel_buffer
    self.sphere_buffer = buffer
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
    local p = vec3(self.camera:pos())
    local e = vec3(self.camera:eye())
    p = p + (p - e) * vec3(self.camera:zoom())
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

local a = 0
local t0_init, t1_init = 60, 60
local t0, t1 = nil, t1_init
function tracer:trace()
    if t0 then
        t0 = t0 - 1
        if t0 == 0 then
            t0 = nil
            t1 = t1_init
        end
    elseif t1 then
        t1 = t1 - 1
        if t1 == 0 then
            t1 = nil
            t0 = t0_init
        else
            self.camera:look_at(
                0, 0, 0, 10 * math.sin(a), 2, 10 * math.cos(a), 0, 1, 0)
            a = a + math.sin(math.pi * 1 - t1 / t1_init) / 32
        end
    end
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
        Compute.BUFFER, self.lambertian_buffer,
        Compute.BUFFER, self.metal_buffer,
        Compute.BUFFER, self.dielectric_buffer,
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
