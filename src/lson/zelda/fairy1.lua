return {
    name = "fairy1",
    collider = {type = Collider.AABB, bb = {-4, -8, 4, -4}},
    renderer = {
        type = Renderer.SPRITE,
        tex = "img/zelda/zelda.png",
        size = {24, 16},
    },
    anim = {sprite = {512//8, 512//16, {
        {{12, 0, 15, 1, 100}, {15, 0, 18, 1, 100}},
        {{12, 1, 15, 2, 100}, {15, 1, 18, 2, 100}}}}},
}
