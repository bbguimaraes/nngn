require "src/lua/path"
local vec3 <const> = require("nngn.lib.math").vec3

require("demos/ray/shirley/common").init {
    impl = "CPP",
    n_threads = 4,
    samples = 500,
    gamma_correction = true,
    init = function(t)
        local n <const> = 11
        t:set_max_lambertians(2 + 2 * n * 2 * n)
        t:set_max_metals(1 + 2 * n * 2 * n)
        t:set_max_dielectrics(1 + 2 * n * 2 * n)
        t:add_sphere(
            {0, -1000, 0}, 1000.0,
            t:add_lambertian({0.5, 0.5, 0.5}))
        t:add_sphere({ 0, 1, 0}, 1, t:add_dielectric(1.5))
        t:add_sphere({-4, 1, 0}, 1, t:add_lambertian({0.4, 0.2, 0.1}))
        t:add_sphere({ 4, 1, 0}, 1, t:add_metal({0.7, 0.6, 0.5}, 0))
        local rand <const> = (function()
            local f, m = Math.rand, nngn.math
            return function() return f(m) end
        end)()
        for a = -n, n - 1 do
            for b = -n, n - 1 do
                local choice <const> = rand()
                local center <const> = vec3(
                    a + 0.9 * rand(),
                    0.2,
                    b + 0.9 * rand())
                if (center - vec3(4, 0.2, 0)):len() <= 0.9 then
                    goto continue
                end
                local m
                if choice < 0.8 then
                    m = t:add_lambertian(
                        {rand() * rand(), rand() * rand(), rand() * rand()})
                elseif choice < 0.95 then
                    m = t:add_metal({
                        rand() / 2 + 0.5,
                        rand() / 2 + 0.5,
                        rand() / 2 + 0.5,
                    }, rand() / 2)
                else
                    m = t:add_dielectric(1.5)
                end
                t:add_sphere(center, 0.2, m)
                ::continue::
            end
        end
    end,
    aperture = 0.1,
    camera = function(c)
        local eye = vec3(13, 2, 3)
        c:set_perspective(true)
        c:set_screen(1200, 1200 * 2 / 3)
        c:set_fov_y(math.pi / 9)
        c:set_zoom((vec3() - eye):len())
        c:look_at(0, 0, 0, eye[1], eye[2], eye[3], 0, 1, 0)
    end,
}
