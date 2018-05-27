return {
    name = "tree0",
    renderer = {
        type = Renderer.SPRITE,
        tex = "img/2d_circle.png", size = {80, 112}, z_off = -44,
        scale = {512//16, 512//16}, coords = {0, 0, 5, 7}},
    collider = {
        type = Collider.BB,
        bb = {-8, -52, 8, -36}, rot = math.rad(45),
    },
}
