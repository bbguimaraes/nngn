require("demos/ray/shirley/common").init {
    impl = "CPP",
    n_threads = 4,
    samples = 16,
    gamma_correction = true,
    init = function(t)
        t:set_max_lambertians(2)
        t:set_max_metals(1)
        t:set_max_dielectrics(2)
        local d <const> = t:add_dielectric(1.5)
        t:add_sphere(
            {0.0, -100.5, -1.0}, 100.0,
            t:add_lambertian({0.8, 0.8, 0.0}))
        t:add_sphere({ 0.0, 0.0, -1.0}, 0.5, t:add_lambertian({0.1, 0.2, 0.5}))
        t:add_sphere({-1.0, 0.0, -1.0}, 0.5, d)
        t:add_sphere({-1.0, 0.0, -1.0}, -0.4, d)
        t:add_sphere({ 1.0, 0.0, -1.0}, 0.5, t:add_metal({0.8, 0.6, 0.2}, 0))
    end,
    aperture = 2,
    camera = function(c)
        c:set_perspective(true)
        c:set_screen(2 * 16 / 9, 2)
        c:set_fov_y(math.pi / 9)
        c:set_zoom(math.sqrt(27))
        c:look_at(0, 0, -1, 3, 3, 2, 0, 1, 0)
    end,
}
