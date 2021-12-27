local utils <const> = require "nngn.lib.utils"
local vec3 <const> = require("nngn.lib.math").vec3

local LOCAL_SIZE <const> = 16

local UINT_SIZE <const> = 4
local UINT2_SIZE <const> = 2 * UINT_SIZE
local FLOAT_SIZE <const> = 4
local FLOAT4_SIZE <const> = 4 * FLOAT_SIZE
local FLOAT3_SIZE <const> = FLOAT4_SIZE
local RND_STATE_SIZE <const> = 4
local RND_STATE_BYTES <const> = RND_STATE_SIZE * UINT_SIZE
local LAMBERTIAN_ELEMS <const> = 4
local LAMBERTIAN_SIZE <const> = FLOAT3_SIZE
local METAL_ELEMS <const> = 4
local METAL_SIZE <const> = FLOAT3_SIZE
local DIELECTRIC_ELEMS <const> = 1
local DIELECTRIC_SIZE <const> = FLOAT_SIZE
local AABB_SIZE <const> = 2 * FLOAT3_SIZE
local BVH_SIZE <const> = AABB_SIZE
local SPHERE_ELEMS <const> = 1
local SPHERE_SIZE <const> = 2 * FLOAT4_SIZE
local CAMERA_BUFFER_SIZE <const> = 7 * FLOAT3_SIZE
local CONF_BUFFER_SIZE <const> =
    4 * UINT_SIZE
    + 4 * FLOAT_SIZE
    + 3 * FLOAT3_SIZE
    + 1 * UINT2_SIZE
    + 3 * FLOAT_SIZE
    + CAMERA_BUFFER_SIZE

local MATERIAL_TYPE_BITS <const> = 2
local MATERIAL_TYPE_LAMBERTIAN <const> = 0
local MATERIAL_TYPE_METAL <const> = 1
local MATERIAL_TYPE_DIELECTRIC <const> = 2
local MATERIAL_LAMBERTIAN <const> =
    MATERIAL_TYPE_LAMBERTIAN << (32 - MATERIAL_TYPE_BITS)
local MATERIAL_METAL <const> =
    MATERIAL_TYPE_METAL << (32 - MATERIAL_TYPE_BITS)
local MATERIAL_DIELECTRIC <const> =
    MATERIAL_TYPE_DIELECTRIC << (32 - MATERIAL_TYPE_BITS)

local tracer <const> = utils.class "tracer" {}

local function create_prog()
    local f <const> =
        assert(io.open("demos/ray/shirley/shirley.cl")):read("a")
    return assert(nngn.compute:create_program(f, table.concat({
        "-Werror",
        "-D BVH_ENABLED=1",
        "-D PI=" .. math.pi .. "f",
        "-D TAU=" .. (2 * math.pi) .. "f",
        "-D MATERIAL_TYPE_LAMBERTIAN=" .. MATERIAL_TYPE_LAMBERTIAN .. "U",
        "-D MATERIAL_TYPE_METAL=" .. MATERIAL_TYPE_METAL .. "U",
        "-D MATERIAL_TYPE_DIELECTRIC=" .. MATERIAL_TYPE_DIELECTRIC .. "U",
        "-D MATERIAL_TYPE_BITS=" .. MATERIAL_TYPE_BITS .. "U",
        "-D SKY_TOP=((float3){1,1,1})",
        "-D SKY_BOTTOM=((float3){0.5f,0.7f,1.0f})",
    }, " ")))
end

local function create_buffer(t, n_elements, bytes, f)
    local n = math.max(#t // n_elements, 1)
    assert(#t == 0 or #t == n * n_elements)
    -- XXX
    n = 2 ^ math.ceil(math.log(n, 2))
    bytes = n * bytes
    local v <const> = Compute.create_vector(bytes)
    assert(f(v))
    return assert(nngn.compute:create_buffer(
        Compute.READ_ONLY, Compute.BYTEV, bytes, v))
end

local function update_spheres(t)
    local size <const> = SPHERE_SIZE
    return create_buffer(t, SPHERE_ELEMS, size, function(v)
        local w <const> = {
            Compute.FLOATV, nil,
            Compute.UINT, nil,
            Compute.FLOATV, {0, 0, 0},
        }
        for i, x in ipairs(t) do
            w[2], w[4] = x[1], x[2]
            if not Compute.write_vector(v, size * (i - 1), w) then
                return false
            end
        end
        return true
    end)
end

local function update_bvh(spheres)
    local n_spheres <const> = #spheres / SPHERE_ELEMS
    local levels <const> = math.ceil(math.log(n_spheres, 2))
    local n <const> = 2 ^ (levels + 1)
    local bytes <const> = BVH_SIZE * n
    local buf <const> = assert(nngn.compute:create_buffer(
        Compute.READ_ONLY, Compute.BYTEV, bytes))
    assert(nngn.compute:fill_buffer(buf, 0, bytes, Compute.BYTEV, {0}))
    return buf
end

local function update_data(lambertians, metals, dielectrics, spheres)
    return
        create_buffer(
            lambertians, LAMBERTIAN_ELEMS, LAMBERTIAN_SIZE, function(v)
                return Compute.write_vector(
                    v, 0, {Compute.FLOATV, lambertians})
            end),
        create_buffer(
            metals, METAL_ELEMS, METAL_SIZE, function(v)
                return Compute.write_vector(v, 0, {Compute.FLOATV, metals})
            end),
        create_buffer(
            dielectrics, DIELECTRIC_ELEMS, DIELECTRIC_SIZE, function(v)
                return Compute.write_vector(
                    v, 0, {Compute.FLOATV, dielectrics})
            end),
        update_spheres(spheres),
        update_bvh(spheres)
end

function tracer:new(gamma_correction)
    return setmetatable({
        n_samples = 0,
        max_samples = 1,
        max_depth = 2,
        updated = true,
        camera_updated = true,
        prog = create_prog(),
        conf_v = Compute.create_vector(CONF_BUFFER_SIZE),
        conf = assert(nngn.compute:create_buffer(
            Compute.READ_WRITE, Compute.BYTEV, CONF_BUFFER_SIZE)),
        gamma_correction = gamma_correction,
        lambertians = {},
        metals = {},
        dielectrics = {},
        spheres = {},
    }, self)
end

function tracer:set_enabled(b) self.enabled = b end

function tracer:set_size(w, h)
    local n <const> = w * h
    local rnd_bytes <const> = RND_STATE_BYTES * n
    local rnd_v <const> = Compute.create_vector(rnd_bytes)
    nngn.math:fill_rnd_vec(0, rnd_bytes, rnd_v)
    local rnd_buf <const> = assert(nngn.compute:create_buffer(
        Compute.READ_WRITE, Compute.BYTEV, rnd_bytes, rnd_v))
    local tex_bytes <const> = 4 * n
    local tex_f_bytes <const> = FLOAT3_SIZE * n
    local tex_f <const> = assert(nngn.compute:create_buffer(
        Compute.READ_WRITE, Compute.FLOATV, tex_f_bytes))
    local tex <const> = assert(nngn.compute:create_buffer(
        Compute.WRITE_ONLY, Compute.BYTEV, tex_bytes))
    nngn.compute:fill_buffer(tex_f, 0, tex_f_bytes, Compute.FLOATV, {0})
    local tex_v <const> = Compute.create_vector(tex_bytes)
    self.w = w
    self.h = h
    self.rnd_buf = rnd_buf
    self.tex = tex
    self.tex_f = tex_f
    self.tex_v = tex_v
end

function tracer:set_camera(c, aperture)
    self.camera = c
    self.lens_radius = aperture / 2
end

function tracer:set_max_depth(n) self.max_depth = n end
function tracer:set_max_samples(n) self.max_samples = n end
function tracer:set_min_t(n) self.t_min = n end
function tracer:set_max_t(n) self.t_max = n end
function tracer:set_max_lambertians() end
function tracer:set_max_metals() end
function tracer:set_max_dielectrics() end

function tracer:add_lambertian(albedo)
    local t <const> = self.lambertians
    local ret <const> = MATERIAL_LAMBERTIAN | (#t // LAMBERTIAN_ELEMS)
    table.insert(t, albedo[1])
    table.insert(t, albedo[2])
    table.insert(t, albedo[3])
    table.insert(t, 0)
    return ret
end

function tracer:add_metal(albedo, fuzz)
    local t <const> = self.metals
    local ret <const> = MATERIAL_METAL | (#t // METAL_ELEMS)
    table.insert(t, albedo[1])
    table.insert(t, albedo[2])
    table.insert(t, albedo[3])
    table.insert(t, fuzz)
    return ret
end

function tracer:add_dielectric(n1_n0)
    local t <const> = self.dielectrics
    local ret <const> = MATERIAL_DIELECTRIC | (#t // DIELECTRIC_ELEMS)
    table.insert(t, n1_n0)
    return ret
end

function tracer:add_sphere(c, r, m)
    table.insert(self.spheres, {{c[1], c[2], c[3], r}, m})
end

function tracer:update(timing)
    if not self.enabled then
        return false
    end
    if self.camera:update(timing) then
        self.camera_updated = true
    elseif self.n_samples == self.max_samples then
        return false
    end
    if not self.sphere_buf then
        self.updated = true
        for _, x in ipairs{
            self.lambertian_buf,
            self.metal_buf,
            self.dielectric_buf,
            self.sphere_buf,
            self.bvh_buf,
        } do
            assert(nngn.compute:release_buffer(x))
        end
        self.lambertian_buf,
            self.metal_buf,
            self.dielectric_buf,
            self.sphere_buf,
            self.bvh_buf = update_data(
                self.lambertians, self.metals, self.dielectrics, self.spheres)
    end
    local events <const> = {}
    if self.updated or self.camera_updated then
        self.n_samples = 0
        local pad <const> = 0
        local c_pos <const> = vec3(self.camera:pos())
        local c_eye <const> = vec3(self.camera:eye())
        local c_w, c_h <const> = self.camera:screen()
        local fov_y <const> = self.camera:fov_y()
        assert(Compute.write_vector(self.conf_v, 0, {
            Compute.UINT, self.w,
            Compute.UINT, self.h,
            Compute.UINT, self.max_depth,
            Compute.UINT, #self.spheres // SPHERE_ELEMS,
            Compute.FLOAT, self.t_min,
            Compute.FLOAT, self.t_max,
            Compute.FLOAT, pad,
            Compute.FLOAT, pad,
            Compute.FLOATV, c_pos,
            Compute.FLOAT, pad,
            Compute.FLOATV, c_eye,
            Compute.FLOAT, pad,
            Compute.FLOATV, {self.camera:up()},
            Compute.FLOAT, pad,
            Compute.UINT, c_w,
            Compute.UINT, c_h,
            Compute.FLOAT, fov_y,
            Compute.FLOAT, self.lens_radius,
            Compute.FLOAT, self.camera:zoom(),
        }))
        assert(nngn.compute:write_buffer(
            self.conf, 0, CONF_BUFFER_SIZE, Compute.BYTEV, self.conf_v))
        assert(nngn.compute:execute(self.prog, "camera", 0, {1}, {1}, {
            Compute.BUFFER, self.conf,
        }, nil, events))
        if self.updated then
            assert(nngn.compute:execute(self.prog, "bvh", 0, {1}, {1}, {
                Compute.BUFFER, self.rnd_buf,
                Compute.UINT, #self.spheres // SPHERE_ELEMS,
                Compute.BUFFER, self.sphere_buf,
                Compute.BUFFER, self.bvh_buf,
            }, nil, events))
        end
        self.updated = false
        self.camera_updated = false
    end
    assert(nngn.compute:execute(
        self.prog, "trace", 0,
        {self.w, self.h}, {LOCAL_SIZE, LOCAL_SIZE}, {
            Compute.BUFFER, self.conf,
            Compute.UINT, self.n_samples,
            Compute.BUFFER, self.rnd_buf,
            Compute.BUFFER, self.lambertian_buf,
            Compute.BUFFER, self.metal_buf,
            Compute.BUFFER, self.dielectric_buf,
            Compute.BUFFER, self.bvh_buf,
            Compute.BUFFER, self.sphere_buf,
            Compute.BUFFER, self.tex_f,
        }, events, events))
    assert(nngn.compute:execute(
        self.prog, "write_tex", 0,
        {self.h}, {LOCAL_SIZE * LOCAL_SIZE}, {
            Compute.UINT, self.w,
            Compute.BUFFER, self.tex_f,
            Compute.BUFFER, self.tex,
        }, events, events))
    assert(nngn.compute:wait(events))
    assert(nngn.compute:release_events(events))
    self.n_samples = self.n_samples + 1
    return true
end

function tracer:write_tex(v)
    assert(nngn.compute:read_buffer(
        self.tex, Compute.BYTEV, Compute.vector_size(v), v))
end

return {
    tracer = tracer,
}
