local utils <const> = require "nngn.lib.utils"
local vec3 <const> = require("nngn.lib.math").vec3
local rand = (function(m, f)
    return function() return f(m) end
end)(nngn.math, nngn.math.rand)

local ray <const> = utils.class "ray" {}
local hit <const> = utils.class "hit" {}
local sphere <const> = utils.class "sphere" {}
local lambertian <const> = utils.class "lambertian" {}
local metal <const> = utils.class "metal" {}
local dielectric <const> = utils.class "dielectric" {}
local camera <const> = utils.class "camera" {}
local tracer <const> = utils.class "tracer" {}

local COLOR_MUL <const> = vec3(255.9)
local TO_INT_VEC <const> = vec3(1)
local SKY_TOP, SKY_BOTTOM <const> = vec3(.5, .7, 1), vec3(1)
local UNIT_SPHERE

local function sky(r)
    return SKY_BOTTOM:interp(SKY_TOP, .5 * (r.d:norm()[2] + 1))
end

local function color(l, r, t_min, t_max, depth, max_depth)
    if depth == max_depth then return vec3() end
    local h = hit.closest(l, r, t_min, t_max)
    if h then
        local s, att = h.mat:scatter(r, h)
        if not s then return vec3() end
        return att * color(l, s, t_min, t_max, depth + 1, max_depth)
    end
    return sky(r)
end

function ray:new(p, d) return setmetatable({p = p, d = d}, self) end
function ray:at(t) return self.p + self.d * vec3(t) end

function hit:new(t, p, n, mat) return {t = t, p = p, n = n, mat = mat} end

function hit.closest(l, r, t_min, t_max)
    local ret = nil
    for _, x in ipairs(l) do
        local h = x:hit(r, t_min, t_max)
        if h then ret, t_max = h, h.t end
    end
    return ret
end

function sphere:new(c, r, mat)
    return setmetatable({
        c = c,
        r = r,
        mat = mat,
    }, self)
end

function sphere:random_point()
    while true do
        local p = vec3(rand(), rand(), rand())
        if p:len() < 1 then return p end
    end
end

function sphere:hit(r, t_min, t_max)
    p = r.p - self.c
    local a = r.d:dot(r.d)
    local b = p:dot(r.d)
    local c = p:dot(p) - self.r * self.r
    local disc = b * b - a * c
    if disc <= 0 then return end
    local t = (-b - math.sqrt(disc)) / a
    if t_min < t and t < t_max then
        local hp = r:at(t)
        return hit:new(t, hp, (hp - self.c) / vec3(self.r), self.mat)
    end
    t = (-b + math.sqrt(disc)) / a
    if t_min < t and t < t_max then
        local hp = r:at(t)
        return hit:new(t, hp, (hp - self.c) / vec3(self.r), self.mat)
    end
end

function lambertian:new(a) return setmetatable({albedo = a}, self) end

function lambertian:random()
    return self:new{rand() * rand(), rand() * rand(), rand() * rand()}
end

function lambertian:scatter(r, h)
    return ray:new(h.p, h.n + UNIT_SPHERE:random_point()), self.albedo
end

function metal:new(a, fuzz)
    return setmetatable({
        albedo = a,
        fuzz = vec3(math.min(fuzz or 1, 1)),
    }, self)
end

function metal:random()
    return self:new(
        {.5 * (1 + rand()), .5 * (1 + rand()), .5 * (1 + rand())},
        .5 * rand())
end

function metal:scatter(r, h)
    return
        ray:new(h.p,
            (-r.d:norm()):reflect(h.n)
            + self.fuzz * UNIT_SPHERE:random_point()),
        self.albedo
end

function dielectric:new(n) return setmetatable({n = n}, self) end

function dielectric.shlick(cos, n)
    local r0 = (1 - n) / (1 + n)
    r0 = r0 * r0
    r0 = r0 + (1 - r0) * math.pow(1 - cos, 5)
    return r0
end

function dielectric:scatter(r, h)
    local cos
    local n0, n1 = 1, 1
    local dot = r.d:dot(h.n)
    if dot > 0 then
        n1, out_n = self.n, -h.n
        cos = self.n * dot / r.d:len()
    else
        n0, out_n = self.n, h.n
        cos = -dot / r.d:len()
    end
    local refr = r.d:refract(out_n, n0, n1)
    local refr_p
    if refr then
        refr_p = self.shlick(cos, self.n)
    else
        refr_p = 1
    end
    if rand() < refr_p then
        return ray:new(h.p, (-r.d):reflect(h.n)), {1, 1, 1}
    else
        return ray:new(h.p, refr), {1, 1, 1}
    end
end

function camera:new()
    return setmetatable({}, self)
end

function camera:init(fov_y, aspect, aperture, focus_dist)
    self.fov_y = fov_y
    self.aspect = aspect
    self.lens_radius = aperture / 2
    self.lens_radius_v = vec3(aperture / 2)
    self.focus_dist = focus_dist
end

function camera:set_pos(pos, eye, up)
    local half_h <const> = math.tan(self.fov_y / 2)
    local half_w <const> = self.aspect * half_h
    self.w = (eye - pos):norm()
    self.u = up:cross(self.w):norm()
    self.v = self.w:cross(self.u)
    self.o = eye
    self.hor = self.u * vec3(2 * half_w * self.focus_dist)
    self.ver = self.v * vec3(2 * half_h * self.focus_dist)
    self.bl = eye
        - self.hor / vec3(2)
        - self.ver / vec3(2)
        - self.w * vec3(self.focus_dist)
end

function camera:ray(u, v)
    local rd = self.lens_radius_v * UNIT_SPHERE:random_point()
    local off = self.u * vec3(rd[1]) + self.v * vec3(rd[2])
    return ray:new(
        self.o + off,
        self.bl
            + self.hor * vec3(u)
            + self.ver * vec3(v)
            - self.o - off)
end

function tracer:new(gamma_correction)
    local ret = {
        n_samples = 0,
        max_samples = 1,
        gamma_correction = gamma_correction,
        world = {},
    }
    return setmetatable(ret, self)
end

function tracer:set_enabled(b) self.enabled = b end
function tracer:set_max_depth(n) self.max_depth = n end
function tracer:set_max_samples(n) self.max_samples = n end
function tracer:set_min_t(n) self.t_min = n end
function tracer:set_max_t(n) self.t_max = n end
function tracer:set_max_lambertians() end
function tracer:set_max_metals() end
function tracer:set_max_dielectrics() end

function tracer:set_size(w, h)
    local img_pixels <const> = w * h
    local img_bytes <const> = 4 * img_pixels
    local tex <const> = Compute.create_vector(img_bytes)
    Compute.zero_vector(tex, 0, 0)
    self.w = w
    self.h = h
    self.tex = tex
    self.tex_cache = Compute.read_vector(tex, 0, 0, Compute.BYTEV)
    self.tex_cache_f = Compute.read_vector(tex, 0, 0, Compute.FLOATV)
end

function tracer:set_camera(camera_, aperture)
    local screen <const> = {camera_:screen()}
    local camera <const> = camera:new()
    camera:init(camera_:fov_y(), screen[1] / screen[2], 0, camera_:zoom())
    camera:set_pos(vec3(camera_:pos()), vec3(camera_:eye()), vec3(camera_:up()))
    self.camera_ = camera_
    self.camera = camera
    self.aperture = aperture
end

function tracer:add_sphere(...) table.insert(self.world, sphere:new(...)) end
function tracer:add_lambertian(...) return lambertian:new(...) end
function tracer:add_metal(...) return metal:new(...) end
function tracer:add_dielectric(...) return dielectric:new(...) end

function tracer:read_pixel(x, y)
    local i = 4 * (self.w * y + x)
    local t = self.tex_cache_f
    return vec3(t[i + 1], t[i + 2], t[i + 3])
end

function tracer:write_pixel(x, y, c)
    local i = 4 * (self.w * y + x)
    local bi = 4 * (self.w * (self.h - y - 1) + x)
    local b = c
    if self.gamma_correction then
        b = b:sqrt()
    end
    b = b * COLOR_MUL // TO_INT_VEC
    b[4] = 255
    table.move(b, 1, 4, bi + 1, self.tex_cache)
    table.move(c, 1, 3, i + 1, self.tex_cache_f)
end

function tracer:merge_pixel(x, y, p, i, c)
    local pc <const> = self:read_pixel(x, y, self.tex_cache_f)
    return (p * pc + c) / i
end

function tracer:trace(timing)
    if not self.enabled then
        return
    end
    if self.camera_:update(timing) then
        self.n_samples = 0
        local screen <const> = {self.camera_:screen()}
        self.camera:init(
            self.camera_:fov_y(), screen[1] / screen[2], self.aperture,
            self.camera_:zoom())
        self.camera:set_pos(
            vec3(self.camera_:pos()), vec3(self.camera_:eye()),
            vec3(self.camera_:up()))
    end
    if self.n_samples == self.max_samples then
        return false
    end
    local v_p, v_i = vec3(self.n_samples), vec3(self.n_samples + 1)
    local w <const>, h <const> = self.w - 1, self.h - 1
    for y = 0, h do
        for x = 0, w do
            local v = (y + rand()) / h
            local u = (x + rand()) / w
            local c = color(
                self.world, self.camera:ray(u, v),
                self.t_min, self.t_max, 0, self.max_depth)
            c = self:merge_pixel(x, y, v_p, v_i, c)
            self:write_pixel(x, y, c)
        end
    end
    Compute.write_vector(self.tex, 0, {Compute.BYTEV, self.tex_cache})
    self.n_samples = self.n_samples + 1
    return true
end

UNIT_SPHERE = sphere:new(vec3(), 1)

return {
    tracer = tracer,
    camera = camera,
    sphere = sphere,
    lambertian = lambertian,
    metal = metal,
    dielectric = dielectric,
}
