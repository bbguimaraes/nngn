require("demos/ray/shirley/common").init {
    impl = "CPP",
    n_threads = 4,
    init = function(t)
        t:add_sphere({0, 0, -1}, 0.5, t:add_lambertian({1, 0, 0}))
    end,
    camera = function(c)
        c:set_perspective(true)
        c:set_screen(2 * 16 / 9, 2)
        c:set_fov_y(math.pi / 2)
        c:look_at(0, 0, -1, 0, 0, 0, 0, 1, 0)
    end,
}
