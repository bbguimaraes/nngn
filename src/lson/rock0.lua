return {
    name = "rock0",
    renderer = {
        type = Renderer.SPRITE,
        tex = "img/2d_circle.png", size = {32, 64}, z_off = -20,
        scale = {512//32, 512//64}, coords = {5, 0}},
    collider = {
        type = Collider.BB,
        bb = {-12, -28, 8, -10}, rot = math.rad(45)},
}
