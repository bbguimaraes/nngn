require("demos/ray/shirley/common").init {
    impl = "CPP",
    n_threads = 4,
    samples = 16,
    gamma_correction = true,
    init = function(t)
        t:set_max_lambertians(2)
        t:set_max_metals(2)
        t:add_sphere(
            {0.0, -100.5, -1.0}, 100.0,
            t:add_lambertian({0.8, 0.8, 0.0}))
        t:add_sphere({ 0.0, 0.0, -1.0}, 0.5, t:add_lambertian({0.7, 0.3, 0.3}))
        t:add_sphere({-1.0, 0.0, -1.0}, 0.5, t:add_metal({0.8, 0.8, 0.8}, 0.3))
        t:add_sphere({ 1.0, 0.0, -1.0}, 0.5, t:add_metal({0.8, 0.6, 0.2}, 1.0))
    end,
    camera = function(c)
        c:set_perspective(true)
        c:set_screen(2 * 16 / 9, 2)
        c:set_fov_y(math.pi / 2)
        c:look_at(0, 0, -1, 0, 0, 0, 0, 1, 0)
    end,
}
