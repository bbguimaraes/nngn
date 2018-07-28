return {
    name = "cow",
    collider = {type = Collider.AABB, bb = {-8, -8, 8, 0}},
    renderer = {
        type = Renderer.SPRITE,
        tex = "img/harvest_moon.png",
        size = {32.0, 24.0},
    },
    anim = {sprite = {512//16, 512//8, {{
        {2, 58, 4, 61, 2000},
        {2, 61, 4, 64, 150}, {4, 61, 6, 64, 150},
        {2, 61, 4, 64, 150}, {4, 61, 6, 64, 150},
        {2, 58, 4, 61, 2000},
        {4, 58, 6, 61, 500}}}}},
}
