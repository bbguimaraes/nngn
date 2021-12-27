require "src/lua/path"
local vec3 <const> = require("nngn.lib.math").vec3

require("demos/ray/shirley/common").init {
    impl = "CPP",
    n_threads = 4,
    samples = 1024,
    size = {512, 512},
    gamma_correction = true,
    init = function(t)
        t:set_max_lambertians(1)
        local n <const> = 8
        local m <const> = t:add_lambertian{1, 0, 0}
        for z = 0, n - 1 do
            for y = 0, n - 1 do
                for x = 0, n - 1 do
                    t:add_sphere({
                        2 * (0.5 + x - n / 2),
                        2 * (0.5 + y - n / 2),
                        2 * (0.5 + z - n / 2),
                    }, 0.5, m)
                end
            end
        end
    end,
    camera = function(c)
        c:set_perspective(true)
        c:set_screen(1, 1)
        c:set_fov_y(math.pi / 2)
        c:look_at(0, 0, 0, 0, 0, 16, 0, 1, 0)
    end,
}
