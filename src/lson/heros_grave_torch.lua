return {
    name = "heros_grave_torch",
    collider = {type = Collider.AABB, bb = {-9, -16, 9, -1}},
    renderer = {
        type = Renderer.SPRITE,
        tex = "img/chrono_trigger/fionas_forest.png",
        size = {24, 32},
    },
    anim = {sprite = {512//8, 512//32, {{
        {48, 8, 51, 9, 120}, {51, 8, 54, 9, 120},
        {54, 8, 57, 9, 120}, {57, 8, 60, 9, 120}}}}},
}
