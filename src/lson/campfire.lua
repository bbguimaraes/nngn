return {
    name = "campfire",
    collider = {type = Collider.AABB, bb = {-16, -24, 16, -12}},
    renderer = {
        type = Renderer.SPRITE,
        tex = "img/chrono_trigger/fionas_forest.png",
        size = {32, 48},
    },
    anim = {sprite = {512//32, 512//16, {{
        { 8, 16,  9, 19, 120}, { 9, 16, 10, 19, 120},
        {10, 16, 11, 19, 120}, {11, 16, 12, 19, 120}}}}},
}
