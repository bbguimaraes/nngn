return {
    renderer = {
        type = Renderer.SPRITE,
        tex = "img/zelda/zelda.png",
        size = {16, 16},
        scale = {512//16, 512//16},
        coords = {14, 0},
    },
    anim = {
        sprite = {512//16, 512//16, {{
            {14, 0, 20}, {15, 0, 20}, {16, 0, 20},
            {17, 0, 20}, {18, 0, 20}, {19, 0, 1000},
        }}},
    },
}
