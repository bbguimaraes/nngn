dofile("src/lua/path.lua")

local a = 0
local t0_init, t1_init = 60, 60
local t0, t1 = nil, t1_init
local camera
nngn.schedule:next(Schedule.HEARTBEAT, function()
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
            camera:look_at(
                0, 0, 0, 10 * math.sin(a), 2, 10 * math.cos(a), 0, 1, 0)
            a = a + math.sin(math.pi * 1 - t1 / t1_init) / 32
        end
    end
end)

require("demos/ray/shirley/common").init {
    impl = "CL",
    n_threads = 4,
    samples = 1 << 20,
    gamma_correction = true,
    init = function(t)
        local n <const> = 2
        t:set_max_lambertians(2 + 4 * n * n)
        t:set_max_metals(1 + 4 * n * n)
        t:set_max_dielectrics(1 + 4 * n * n)
        local rand = (function()
            local f, m = Math.rand, nngn.math
            return function() return f(m) end
        end)()
        local nngn_math = require("nngn.lib.math")
        local vec3, vec3_rand = nngn_math.vec3, nngn_math.vec3_rand
        t:add_sphere({0, -1000, 0}, 1000, t:add_lambertian({0.5, 0.5, 0.5}))
        t:add_sphere({0, 1, 0}, 1, t:add_dielectric(1.5))
        t:add_sphere({-4, 1, 0}, 1, t:add_lambertian({0.4, 0.2, 0.1}))
        t:add_sphere({4, 1, 0}, 1, t:add_metal({0.7, 0.6, 0.5}, 0))
        for a = -n, n - 1 do
            for b = -n, n - 1 do
                local center = vec3(a + 0.9 * rand(), 0.2, b + 0.9 * rand())
                if center:len_sq() <= 0.9 * 0.9 then
                    goto continue
                end
                local mat = rand()
                if mat < 0.8 then
                    mat = t:add_lambertian(vec3_rand(rand) * vec3_rand(rand))
                elseif mat < 0.95 then
                    mat = t:add_metal(
                        vec3_rand(rand) / vec3(2) + vec3(0.5), rand(0, 0.5))
                else
                    mat = t:add_dielectric(1.5)
                end
                t:add_sphere(center, 0.2, mat)
                ::continue::
            end
        end
    end,
    aperture = 0.1,
    camera = function(c)
        camera = c
        c:set_perspective(true)
        c:set_max_vel(4)
        c:set_screen(512, 512)
        c:look_at(0, 0, 0, 13, 2, 3, 0, 1, 0)
        c:set_zoom(10)
        c:set_fov_y(math.pi / 9)
    end,
}
