return {
    name = "fairy2",
    collider = {type = Collider.AABB, bb = {-4, -34, 4, -28}},
    renderer = {
        type = Renderer.SPRITE,
        tex = "img/zelda.png",
        size = {16, 16},
    },
    anim = {sprite = {512//16, 512//16, {{
        {0, 18, 80}, { 1, 18, 80}, {2, 18, 80}, { 3, 18, 80}, {4, 18, 80},
        {5, 18, 80}, { 6, 18, 80}, {7, 18, 80}, { 8, 18, 80}, {6, 18, 80},
        {9, 18, 80}, {10, 18, 80}, {1, 18, 80}, {11, 18, 80}}}}},
}
