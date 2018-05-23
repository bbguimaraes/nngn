return {
    name = "fairy2",
    collider = {type = Collider.AABB, bb = {-4, -34, 4, -28}},
    renderer = {
        type = Renderer.SPRITE,
        tex = "img/zelda/zelda.png",
        size = {16, 16},
        scale = {512//16, 512//16},
        coords = {22, 0},
    },
    anim = {sprite = {512//16, 512//16, {{
        {20, 0, 80}, {21, 0, 80}, {22, 0, 80}, {23, 0, 80}, {24, 0, 80},
        {25, 0, 80}, {26, 0, 80}, {27, 0, 80}, {28, 0, 80}, {26, 0, 80},
        {29, 0, 80}, {30, 0, 80}, {21, 0, 80}, {31, 0, 80},
    }}}},
}
