dofile("src/lua/path.lua")

local vec3 = require("nngn.lib.math").vec3
local rand = (function()
    local m = nngn.math
    local r = m.rand
    return function() return r(m) end
end)()

local IMG_SIZE = 512
local IMG_BYTES = 4 * IMG_SIZE * IMG_SIZE
local TEX_SIZE = 512
local TEX_BYTES = 4 * TEX_SIZE * TEX_SIZE
local SAMPLES = 1024
local MAX_DEPTH = 32
local SKY_TOP, SKY_BOTTOM = vec3(.5, .7, 1), vec3(1)
local COLOR_MUL = vec3(255.9)
local TO_INT_VEC = vec3(1)

local ray = {}
ray.__index = ray
function ray:new(p, d) return setmetatable({p = p, d = d}, self) end
function ray:point_at(t) return self.p + self.d * vec3(t) end

local hit = {}
function hit:new(t, p, n, mat) return {t = t, p = p, n = n, mat = mat} end

function hit.closest(l, r, t_min, t_max)
    local ret = nil
    for _, x in ipairs(l) do
        local h = x:hit(r, t_min, t_max)
        if h then ret, t_max = h, h.t end
    end
    return ret
end

local sphere = {}
sphere.__index = sphere

function sphere:new(c, r, mat)
    return setmetatable({
        c = c,
        r = r,
        rv = vec3(r),
        mat = mat,
    }, self)
end

local unit_sphere = sphere:new(vec3(), 1)

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
        local hp = r:point_at(t)
        return hit:new(t, hp, (hp - self.c) / self.rv, self.mat)
    end
    t = (-b + math.sqrt(disc)) / a
    if t_min < t and t < t_max then
        local hp = r:point_at(t)
        return hit:new(t, hp, (hp - self.c) / self.rv, self.mat)
    end
end

local function sky(r)
    return SKY_BOTTOM:interp(SKY_TOP, .5 * (r.d:norm()[2] + 1))
end

local function color(l, r, t_min, t_max, depth)
    if depth == MAX_DEPTH then return vec3() end
    local h = hit.closest(l, r, t_min, t_max)
    if h then
        local s, att = h.mat:scatter(r, h)
        if not s then return vec3() end
        return att * color(l, s, t_min, t_max, depth + 1)
    end
    return sky(r)
end

local lambertian = {}
lambertian.__index = lambertian

function lambertian:new(a) return setmetatable({albedo = a}, self) end

function lambertian:random()
    return self:new{rand() * rand(), rand() * rand(), rand() * rand()}
end

function lambertian:scatter(r, h)
    return ray:new(h.p, h.n + unit_sphere:random_point()), self.albedo
end

local metal = {}
metal.__index = metal

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
            + self.fuzz * unit_sphere:random_point()),
        self.albedo
end

local dielectric = {}
dielectric.__index = dielectric
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

local camera = {}

function camera:init(pos, eye, up, fovy, asp, aperture, focus_dist)
    local hh = math.tan(fovy / 2)
    local hw = asp * hh
    self.w = (pos - eye):norm()
    self.u = up:cross(self.w):norm()
    self.v = self.w:cross(self.u)
    self.lens_radius = aperture / 2
    self.lens_radius_v = vec3(self.lens_radius)
    self.o = pos
    self.bl = pos
        - self.u * vec3(hw * focus_dist)
        - self.v * vec3(hh * focus_dist)
        - self.w * vec3(focus_dist)
    self.hor = self.u * vec3(2 * hw * focus_dist)
    self.ver = self.v * vec3(2 * hh * focus_dist)
end

function camera:ray(u, v)
    local rd = self.lens_radius_v * unit_sphere:random_point()
    local off = self.u * vec3(rd[1]) + self.v * vec3(rd[2])
    return ray:new(
        self.o + off,
        self.bl
            + self.hor * vec3(u)
            + self.ver * vec3(v)
            - self.o - off)
end

local world = {}

function world:init(r)
    for a = -r, r - 1 do
        for b = -r, r - 1 do
            local center = vec3(a + .9 * rand(), .2, b + .9 * rand())
            if (center - vec3(4, .2, 0)):len() <= .9 then goto continue end
            local choice = rand()
            if choice < .8 then
                table.insert(self, sphere:new(center, .2, lambertian:random()))
            elseif choice < .95 then
                table.insert(self, sphere:new(center, .2, metal:random()))
            else
                table.insert(self, sphere:new(center, .2, dielectric:new(1.5)))
            end
            ::continue::
        end
    end
    table.insert(self, sphere:new({0, 1, 0}, 1, dielectric:new(1.5)))
    table.insert(self, sphere:new({-4, 1, 0}, 1, lambertian:new({.4, .2, .1})))
    table.insert(self, sphere:new({4, 1, 0}, 1, metal:new({.7, .6, .5}, 0)))
    table.insert(self, sphere:new(
        {0, -1000, 0}, 1000, lambertian:new(vec3(.5))))
end

local tracer = {
    w = TEX_SIZE,
    h = TEX_SIZE,
    i_samples = 0,
    n_samples = SAMPLES,
    t_min = 0.001,
    t_max = 99999999,
}

function tracer:init()
    local tex = Math.vec(IMG_BYTES)
    Compute.fill_vector(tex, 0, IMG_BYTES, Compute.BYTEV, {0})
    self.tex = tex
    self.tex_cache = Compute.read_vector(tex, 0, IMG_BYTES, Compute.BYTEV)
end

function tracer:done() return self.i_samples == self.n_samples end

function tracer:read_pixel(x, y)
    local i = 4 * (IMG_SIZE * y + x)
    return vec3(
        self.tex_cache[i + 1],
        self.tex_cache[i + 2],
        self.tex_cache[i + 3])
end

function tracer:write_pixel(x, y, c)
    local i = 4 * (IMG_SIZE * y + x)
    local bi = 4 * (IMG_SIZE * (IMG_SIZE - y - 1) + x)
    local b = c:sqrt() * COLOR_MUL // TO_INT_VEC
    b[4] = 255
    Compute.write_vector(self.tex, bi, {Compute.BYTEV, b})
    table.move(c, 1, 3, i + 1, self.tex_cache)
end

function tracer:merge_pixel(x, y, p, i, c)
    return (c + self:read_pixel(x, y) * p) / i
end

function tracer:trace()
    local v_p, v_i = vec3(self.i_samples), vec3(self.i_samples + 1)
    for y = 0, self.h - 1 do
        for x = 0, self.w - 1 do
            local v = (y + rand()) / self.h
            local u = (x + rand()) / self.w
            local c = color(
                world, camera:ray(u, v),
                self.t_min, self.t_max, 0, self.max_depth)
            self:write_pixel(x, y, self:merge_pixel(x, y, v_p, v_i, c))
        end
    end
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

camera:init(
    vec3(13, 2, 3), vec3(), vec3(0, 1, 0),
    math.rad(20), tracer.w / tracer.h, .1, 10)
tracer:init()
world:init(1)
local tex = nngn.textures:load_data("lua:ray", tracer.tex)
require("nngn.lib.entity").load(nil, nil, {
    renderer = {
        type = Renderer.SPRITE,  size = {512, 512}, tex = tex}})

local heartbeat = nngn.schedule:next(Schedule.HEARTBEAT, function()
    if tracer:done() then
        nngn.schedule:cancel(heartbeat)
    else
        tracer:trace()
        nngn.textures:update_data(tex, tracer.tex)
    end
end)
