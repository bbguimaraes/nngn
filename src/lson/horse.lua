return {
    name = "horse",
    collider = {type = Collider.AABB, bb = {-8, -8, 8, 0}},
    renderer = {
        type = Renderer.SPRITE,
        tex = "img/harvest_moon.png",
        size = {32, 24},
    },
    anim = {sprite = {512//32, 512//8, {{
        {3, 58, 4, 61, 200}, {4, 58, 5, 61, 200},
        {3, 58, 4, 61, 200}, {5, 58, 6, 61, 200}}}}},
}
