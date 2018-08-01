return {
    name = "old_man",
    collider = {type = Collider.AABB, bb = {-6, -16, 6, -8}},
    renderer = {
        type = Renderer.SPRITE,
        size = {16.0, 48.0}, z_off = -14,
        coords = {0, 0, 1, 3},
        scale = {512//16, 512//16},
        tex = "img/chrono_trigger/end_of_time.png",
    },
}
