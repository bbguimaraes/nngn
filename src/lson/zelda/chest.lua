return {
    name = "chest",
    collider = {type = Collider.AABB, bb = {-8, -8, 8, 8}},
    renderer = {
        type = Renderer.SPRITE,
        tex = "img/zelda/zelda.png",
        size = {16, 16},
        scale = {512//16, 512//16},
        coords = {2, 0},
    },
}
