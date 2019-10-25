local camera = require "nngn.lib.camera"
local entity = require "nngn.lib.entity"
local light = require "nngn.lib.light"
local map = require "nngn.lib.map"

local state = {perspective = true, ambient_light = true, shadows_enabled = true}
local entities = {}
local lights = {}

local function init()
    for _, t in ipairs{{
        -- cube
        pos = {0, 0, 5},
        renderer = {
            type = Renderer.MODEL, flags = Renderer.MODEL_DEDUP,
            scale = {10, 10, 10},
            obj = "obj/cube.obj", tex = "obj/img/white.png",
        },
    }, {
        -- human
        renderer = {
            type = Renderer.MODEL,
            flags = Renderer.MODEL_DEDUP | Renderer.MODEL_CALC_NORMALS,
            obj = "obj/human.obj", tex = "obj/img/white.png",
        },
    }, {
        -- animals
        pos = {-192, 0, 0},
        renderer = {
            type = Renderer.MODEL,
            flags = Renderer.MODEL_DEDUP | Renderer.MODEL_CALC_NORMALS,
            obj = "obj/tiger.obj", tex = "obj/img/white.png",
            scale = .1, rot = {.863, .357, .357, math.rad(98.4)},
        },
    }, {
        pos = {-128, 0, 0},
        renderer = {
            type = Renderer.MODEL,
            flags = Renderer.MODEL_DEDUP | Renderer.MODEL_CALC_NORMALS,
            scale = .05, rot = {1, -1, -1, math.rad(120)},
            obj = "obj/deer0.obj", tex = "obj/img/white.png",
        },
    }, {
        pos = {-80, 0, 0},
        renderer = {
            type = Renderer.MODEL,
            flags = Renderer.MODEL_DEDUP | Renderer.MODEL_CALC_NORMALS,
            obj = "obj/deer1.obj", tex = "obj/img/white.png",
            scale = .15, rot = {1, 0, 0, math.rad(90)},
        },
    }, {
        pos = {-32, 0, 0},
        renderer = {
            type = Renderer.MODEL,
            flags = Renderer.MODEL_DEDUP | Renderer.MODEL_CALC_NORMALS,
            obj = "obj/wolf0.obj", tex = "obj/img/white.png",
            scale = .075, rot = {1, -1, -1, math.rad(120)},
        },
    }, {
        pos = {32, 0, 0},
        renderer = {
            type = Renderer.MODEL,
            flags = Renderer.MODEL_DEDUP | Renderer.MODEL_CALC_NORMALS,
            obj = "obj/wolf1.obj", tex = "obj/img/white.png",
            scale = .2, rot = {1, 0, 0, math.rad(90)},
        },
    }, {
        pos = {64, 0, 0},
        renderer = {
            type = Renderer.MODEL,
            flags = Renderer.MODEL_DEDUP | Renderer.MODEL_CALC_NORMALS,
            obj = "obj/cat.obj", tex = "obj/img/white.png",
            scale = .05, rot = {1, -1, -1, math.rad(120)},
        },
    }, {
        pos = {96, 0, 0},
        renderer = {
            type = Renderer.MODEL,
            flags = Renderer.MODEL_DEDUP | Renderer.MODEL_CALC_NORMALS,
            obj = "obj/goat.obj", tex = "obj/img/white.png",
            scale = .075, rot = {1, -1, -1, math.rad(120)},
        },
    }, {
        pos = {144, 0, 0},
        renderer = {
            type = Renderer.MODEL,
            flags = Renderer.MODEL_DEDUP | Renderer.MODEL_CALC_NORMALS,
            obj = "obj/cow.obj", tex = "obj/img/white.png",
            scale = .05, rot = {1, -1, -1, math.rad(120)},
        },
    }, {
        pos = {192, 64, 30},
        renderer = {
            type = Renderer.MODEL,
            flags = Renderer.MODEL_DEDUP | Renderer.MODEL_CALC_NORMALS,
            obj = "obj/dragon.obj", tex = "obj/img/white.png",
            scale = .5, rot = {1, 0, 0, math.rad(90)},
        },
    }} do
        table.insert(entities, entity.load(nil, nil, t))
    end
    -- light
    for _, c in ipairs({{1, 0, 0, 1}, {0, 1, 0, 1}, {0, 0, 1, 1}}) do
        local e = entity.load(nil, nil, {
            light = {type = Light.POINT, color = c, spec = 0, att = 1024}})
        table.insert(entities, e)
        table.insert(lights,
            {e, (math.random() - .5) * 2 * math.pi, 1 - math.random() / 4})
    end
    nngn:camera():set_perspective(true)
    nngn:renderers():set_perspective(true)
    camera.reset(2)
    nngn:lighting():set_ambient_light(0.15, 0.15, 0.15, 1)
    nngn:lighting():set_shadows_enabled(true)
    nngn:lighting():set_shadow_map_far(512)
    light.sun(true)
end

local function heartbeat()
    local dt = nngn:timing():fdt_s()
    local d = 320
    for i, t in ipairs(lights) do
        local l, a, v = table.unpack(t)
        a = a + dt * v
        lights[i][2] = a
        l:set_pos(d * math.cos(a), -96, 32) --d * math.sin(light_a), 16)
    end
end

map.map {
    name = "models",
    state = state,
    init = init,
    entities = entities,
    heartbeat = heartbeat,
    tiles = {
        nngn:textures():load("obj/img/white.png"),
        512, 0, 0, 1024, 1024, 1, 1, {0, 0},
    },
}
