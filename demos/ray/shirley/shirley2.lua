require("demos/ray/shirley/common").init {
    impl = "CPP",
    n_threads = 4,
    samples = 16,
    gamma_correction = true,
    init = function(t)
        t:set_max_lambertians(2)
        t:add_sphere({0, 0, -1}, 0.5, t:add_lambertian({0.5, 0.5, 0.5}))
        t:add_sphere({0, -100.5, -1}, 100, t:add_lambertian({0.5, 0.5, 0.5}))
    end,
    camera = function(c)
        c:set_perspective(true)
        c:set_screen(2 * 16 / 9, 2)
        c:set_fov_y(math.pi / 2)
        c:look_at(0, 0, -1, 0, 0, 0, 0, 1, 0)
    end,
}
