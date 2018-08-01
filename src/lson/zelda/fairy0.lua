return {
    name = "fairy0",
    collider = {type = Collider.AABB, bb = {-4, -8, 4, -4}},
    renderer = {
        type = Renderer.SPRITE,
        tex = "img/zelda/zelda.png",
        size = {8, 16},
    },
    anim = {sprite = {512//8, 512//16, {{{8, 0, 100}, {9, 0, 100}}}}},
}
