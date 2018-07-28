return {
    name = "bird",
    collider = {type = Collider.AABB, bb = {-8, -8, 8, 0}},
    renderer = {
        type = Renderer.SPRITE,
        tex = "img/harvest_moon.png",
        size = {16.0, 16.0},
    },
    anim = {sprite = {512//16, 512//16, {{
        {0, 30, 200}, {1, 30, 200}, {0, 30, 200}, {1, 30, 200},
        {0, 31, 200}, {1, 31, 200}, {0, 31, 200}, {1, 31, 200}}}}},
}
