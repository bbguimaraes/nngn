require "src/lua/path"
local vec3 <const> = require("nngn.lib.math").vec3

local n <const> = 11
local sr <const> = 0.5
local h0 <const>, n0 <const>, r0 <const> = 6, 5, 0.5
local h1 <const>, n1 <const>, r1 <const> = 16, 50, 6
local h2 <const> = 1.5 * sr + (h0 + h1) / 2

local state = {t = 0}

local function smooth(t) return (math.cos(math.pi * (1 + t)) + 1) / 2 end

local function look_at(c, pos, eye)
    c:set_zoom((pos - eye):len())
    c:look_at(pos[1], pos[2], pos[3], eye[1], eye[2], eye[3], 0, 1, 0)
end

local function make_stage(d, s, f)
    return function()
        if state.t > d then
            state.t = state.t - d
            state.stage = s
        elseif f then
            f(state.t / d)
        end
    end
end

local function wait(n, s) return make_stage(n, s) end

local function es() end

local s5 <const> = make_stage(1000, es, function(t)
    state.c:set_zoom(3 + 1024 * t)
    state.c:set_pos(0, h2 + 3 + 1024 * t, 0)
    state.c:set_rot(-math.pi / 2, 4 * math.pi * t, 0)
end)

local s4 <const> = make_stage(500, wait(500, s5), function(t)
    local a = math.pi / 2 * smooth(t)
    look_at(
        state.c, vec3(0, h2, 0),
        vec3(0, h2 + 3 * math.sin(a), 3 * math.cos(a)))
end)

local s3 <const> = make_stage(1500, wait(500, s4), function(t)
    t = smooth(t)
    local a <const> = 2 * math.pi * t
    local y <const> = 2 + (h2 - 2) * t
    local d <const> = 64 - 61 * t
    look_at(state.c, vec3(0, y, 0), vec3(d * math.sin(a), y, d * math.cos(a)))
end)

local s2 <const> = make_stage(500, wait(500, s3), function(t)
    state.c:set_pos(0, h2 - (h2 - 2) * t, 36 + 28 * t)
end)

local s1 <const> = make_stage(500, wait(1000, s2), function(t)
    state.c:set_pos(0, h2, 256 + (36 - 256) * smooth(t))
end)

local function s0()
    state.c:set_zoom(256)
    state.c:look_at(0, h2, 0, 0, h2, 256, 0, 1, 0)
    state.stage = wait(500, s1)
end

i = require("nngn.lib.input")
i.input:remove(32)
i.input:add(32, Input.SEL_PRESS, function()
    nngn.schedule:next(Schedule.HEARTBEAT, function()
        state.t = state.t + nngn.timing:fdt_ms()
        state.stage()
    end)
end)

require("demos/ray/shirley/common").init {
    impl = "CL",
    n_threads = 4,
    samples = 1024,
--    max_depth = 8,
    gamma_correction = true,
    init = function(t)
        t:set_max_lambertians(1 + 3 * 8 + h0 * n0 + h1 * n1 / 2)
        t:set_max_metals(1)
        t:set_max_dielectrics(1 + 2 * n * 2 * n)
        local rand <const>, rand_int <const> = (function()
            local f, fi, m <const> = Math.rand, Math.rand_int, nngn.math
            return
                function() return f(m) end,
                function(...) return fi(m, ...) end
        end)()
        local m0 <const> = t:add_lambertian{0.5, 0, 0}
        local m1 <const> = t:add_lambertian{0, 0.3, 0}
        local colors <const> = {
            {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0},
            {1.0, 0.0, 1.0}, {1.0, 1.0, 1.0}, {1.0, 0.9, 0.0},
        }
        local function circle(p, r, n, m)
            for x = 0, n - 1 do
                local a <const> = 2 * math.pi * x / n
                t:add_sphere(
                    p + {r * math.cos(a), sr, r * math.sin(a)}, sr, m())
            end
        end
        for y = 0, h0 - 1 do
            circle(vec3(0, y / 2, 0), r0, n0, function() return m0 end)
        end
        for y = 0, h1 - 1 do
            local n <const> = n1 * (h1 - y) // h1
            local r <const> = r1 * (h1 - y) / h1
            circle(vec3(0, (h0 + y) / 2, 0), r, n, function()
                local n <const> = rand()
                if n < 0.85 then
                    return m1
                end
                if n < 0.9 then
                    return t:add_dielectric(1 + rand())
                end
                local c <const> = colors[rand_int(1, #colors)]
                if n < 0.95 then
                    local n = rand_int(1, 7)
                    return t:add_lambertian(c)
                else
                    return t:add_metal(c, rand() * 2)
                end
            end)
        end
        t:add_sphere({0, h2, 0}, sr, t:add_metal({1, 0.9, 0}, 0))
        t:add_sphere({0, 0, 0}, 32, t:add_dielectric(1.5))
        t:add_sphere({0, 0, 0}, -31, t:add_dielectric(1.5))
        t:add_sphere(
            {0, -1000, 0}, 1000.0,
            t:add_lambertian{0.8, 0.8, 0.8})
    end,
    aperture = 0.1,
    camera = function(c)
        state.c = c
        c:set_perspective(true)
        c:set_screen(1200, 1200 * 2 / 3)
        c:set_fov_y(math.pi / 9)
        s0(c)
    end,
}
