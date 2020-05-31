local animation = require "nngn.lib.animation"
local camera = require "nngn.lib.camera"
local entity = require "nngn.lib.entity"
local map = require "nngn.lib.map"
local player = require "nngn.lib.player"
local texture = require "nngn.lib.texture"

local entities, collectables = {}, {}
local warp, door, switches, on_switch, anim_switch, anim
local tex <close> = texture.load("img/wolfenstein.png")

local function init()
    -- warp
    table.insert(entities, entity.load(nil, nil, {
        pos = {0, 208},
        collider = {
            type = Collider.AABB, bb = {-96, -16, 96, 16},
            flags = Collider.TRIGGER,
        },
    }))
    warp = entities[#entities]
    -- solids
    for _, t in ipairs{
        {{ -64,  160, 0}, {-32,  -32, 32,  32}},
        {{  64,  160, 0}, {-32,  -32, 32,  32}},
        {{-112,    0, 0}, {-16, -128, 16, 128}},
        {{ 112,    0, 0}, {-16, -128, 16, 128}},
        {{   0, -144, 0}, {-96,  -16, 96,  16}},
    } do
        table.insert(entities, entity.load(nil, nil, {
            pos = t[1],
            collider = {
                type = Collider.AABB,
                bb = t[2], m = Math.INFINITY, flags = Collider.SOLID,
            },
        }))
    end
    -- voxels
    do
        local s16, s32, s512 <const> = 0x1p-4, 0x1p-3, 0x1p-9
        local wall_uv0, wall_uv1 <const> = 4 * s32, (8 - 5) * s32
        local door0_uv_x0, door0_uv_x1 <const> = 8 * s16,  9 * s16
        local door1_uv_x0, door1_uv_x1 <const> = 9 * s16, 10 * s16
        local door_uv_y0, door_uv_y1 <const> = (8 - 2) * s32,  (8 - 3) * s32
        local ceil_uv_x0, ceil_uv_x1 <const> = 1 * s512, 2 * s512
        local ceil_uv_y0, ceil_uv_y1 <const> = 0, 1 * s512
        local wall_uv <const> = {
            wall_uv0, wall_uv0, wall_uv1, wall_uv1,
            wall_uv0, wall_uv0, wall_uv1, wall_uv1,
            wall_uv0, wall_uv0, wall_uv1, wall_uv1,
            wall_uv0, wall_uv0, wall_uv1, wall_uv1,
            wall_uv0, wall_uv0, wall_uv1, wall_uv1,
            wall_uv0, wall_uv0, wall_uv1, wall_uv1,
        }
        local door0_uv <const> = {
            door0_uv_x0, door_uv_y0, door0_uv_x1, door_uv_y1,
            door0_uv_x0, door_uv_y0, door0_uv_x1, door_uv_y1,
            door0_uv_x0, door_uv_y0, door0_uv_x1, door_uv_y1,
            door0_uv_x0, door_uv_y0, door0_uv_x1, door_uv_y1,
            door0_uv_x0, door_uv_y0, door0_uv_x1, door_uv_y1,
            door0_uv_x0, door_uv_y0, door0_uv_x1, door_uv_y1,
        }
        local door1_uv <const> = {
            door1_uv_x0, door_uv_y0, door1_uv_x1, door_uv_y1,
            door1_uv_x0, door_uv_y0, door1_uv_x1, door_uv_y1,
            door1_uv_x0, door_uv_y0, door1_uv_x1, door_uv_y1,
            door1_uv_x0, door_uv_y0, door1_uv_x1, door_uv_y1,
            door1_uv_x0, door_uv_y0, door1_uv_x1, door_uv_y1,
            door1_uv_x0, door_uv_y0, door1_uv_x1, door_uv_y1,
        }
        local ceil_uv <const> = {
            ceil_uv_x0, ceil_uv_y0, ceil_uv_x1, ceil_uv_y1,
            ceil_uv_x0, ceil_uv_y0, ceil_uv_x1, ceil_uv_y1,
            ceil_uv_x0, ceil_uv_y0, ceil_uv_x1, ceil_uv_y1,
            ceil_uv_x0, ceil_uv_y0, ceil_uv_x1, ceil_uv_y1,
            ceil_uv_x0, ceil_uv_y0, ceil_uv_x1, ceil_uv_y1,
            ceil_uv_x0, ceil_uv_y0, ceil_uv_x1, ceil_uv_y1,
        }
        for _, t in ipairs{
            -- walls / solids
            {{ -96,  96, 32}, {0, 64, 64}, wall_uv},
            {{ -96,  32, 32}, {0, 64, 64}, wall_uv},
            {{ -96, -32, 32}, {0, 64, 64}, wall_uv},
            {{ -96, -96, 32}, {0, 64, 64}, wall_uv},
            {{  96,  96, 32}, {0, 64, 64}, wall_uv},
            {{  96,  32, 32}, {0, 64, 64}, wall_uv},
            {{  96, -32, 32}, {0, 64, 64}, wall_uv},
            {{  96, -96, 32}, {0, 64, 64}, wall_uv},
            -- door walls
            {{ -32, 136, 32}, {0, 16, 64}, door0_uv},
            {{  32, 136, 32}, {0, 16, 64}, door1_uv},
            -- ceiling
            {{   0,  16, 64}, {192, 288, 0}, ceil_uv},
        } do
            table.insert(entities, entity.load(nil, nil, {
                pos = t[1],
                renderer = {
                    type = Renderer.VOXEL,
                    tex = tex.tex, size = t[2], uv = t[3],
                },
            }))
        end
    end
    -- walls
    for _, t in ipairs({
        {{-64, 160}, {2, 4}},
        {{ 64, 160}, {2, 4}},
        {{-64, -96}, {2, 4}},
        {{  0, -96}, {2, 4}},
        {{ 64, -96}, {2, 4}},
    }) do
        table.insert(entities, entity.load(nil, nil, {
            pos = t[1],
            renderer = {
                type = Renderer.SPRITE, tex = tex.tex,
                size = {64, 64}, scale = {8, 8}, coords = t[2],
            },
        }))
    end
    -- cubes
    for _, t in pairs({
        {{-24,   0, 6}, {1, 0, 0}},
        {{ 24, -24, 6}, {0, 1, 0}},
        {{ 24,  24, 6}, {0, 0, 1}},
    }) do
        table.insert(entities, entity.load(nil, nil, {
            pos = t[1],
            renderer = {type = Renderer.CUBE, size = 12, color = t[2]},
            collider = {
                type = Collider.AABB, bb = 8, flags = Collider.SOLID}}))
    end
    -- collectables
    for _, t in ipairs({
        {{-64,  64}, {16, 32}, {32, 16}, {24,  0}},
        {{ 64,  64}, {32, 32}, {16, 16}, {13,  0}},
        {{-64,   0}, {16, 32}, {32, 16}, {25,  0}},
        {{ 64,   0}, {64, 16}, { 8, 32}, { 7,  0}},
        {{-64, -64}, {16, 32}, {32, 16}, {24,  0}},
        {{ 64, -64}, {32, 32}, {16, 16}, {13,  0}},
    }) do
        local e = entity.load(nil, nil, {
            pos = t[1],
            renderer = {
                type = Renderer.SPRITE, tex = tex.tex,
                size = t[2], scale = t[3], coords = t[4]},
            collider = {
                type = Collider.AABB, bb = {-4, -20, 4, -12},
                flags = Collider.TRIGGER,
            },
        })
        collectables[deref(e)] = e
        table.insert(entities, e)
    end
    -- lamps
    for _, t in ipairs({
        {{-64, 112,  0}, {32, 32}, {16, 16}, {15, 1}, -15},
        {{ 64, 112,  0}, {32, 32}, {16, 16}, {15, 1}, -15},
    }) do
        table.insert(entities, entity.load(nil, nil, {
            pos = t[1],
            renderer = {
                type = Renderer.SPRITE, tex = tex.tex,
                size = t[2], scale = t[3], coords = t[4], z_off = t[5],
            },
            collider = {
                type = Collider.AABB, bb = {-8, -2 + t[5], 8, 2 + t[5]},
                m = Math.INFINITY, flags = Collider.SOLID,
            },
        }))
    end
    -- lamp lights
    for _, t in ipairs{
        {{-64, 136, 0}, {32, 16}, {32, 32}, {30, 4, 32, 5}, -39},
        {{ 64, 136, 0}, {32, 16}, {32, 32}, {30, 4, 32, 5}, -39},
    } do
        table.insert(entities, entity.load(nil, nil, {
            pos = t[1],
            renderer = {
                type = Renderer.TRANSLUCENT, tex = tex.tex,
                size = t[2], scale = t[3], coords = t[4], z_off = t[5],
            },
        }))
        table.insert(entities, entity.load(nil, nil, {
            pos = {t[1][1], t[1][2] - 42, 34},
            light = {
                type = Light.POINT, color = {.6, 1, .6, 1},
                att = 512, spec = 0,
            },
        }))
    end
    -- door
    table.insert(entities, entity.load(nil, nil, {
        pos = {0, 176},
        renderer = {
            type = Renderer.SPRITE, tex = tex.tex,
            size = {64, 64}, scale = {8, 8}, coords = {2, 2}},
        collider = {
            type = Collider.AABB,
            bb = {-32, -32, 32, -24}, m = Math.INFINITY,
            flags = Collider.SOLID}}))
    door = entities[#entities]
    -- switches
    table.insert(entities, entity.load(nil, nil, {
        pos = {0, 176},
        collider = {
            type = Collider.AABB, bb = {-32, -40, 32, -32},
            flags = Collider.TRIGGER}}))
    table.insert(entities, entity.load(nil, nil, {
        pos = {0, 192},
        collider = {
            type = Collider.AABB, bb = {-32, -40, 32, -32},
            flags = Collider.TRIGGER}}))
    switches = {
        [deref(entities[#entities - 1])] = true,
        [deref(entities[#entities])] = true,
    }
    table.insert(entities, entity.load(nil, nil, {
        pos = {-80, -112},
        collider = {
            type = Collider.AABB, bb = {-16, -16, 16, 16},
            flags = Collider.TRIGGER,
        },
    }))
    anim_switch = entities[#entities]
    camera.set_follow(nil)
    nngn:renderers():set_perspective(true)
    nngn:camera():set_perspective(true)
    nngn:camera():set_rot(math.pi / 2, 0, 0)
    nngn:camera():set_zoom(1)
    nngn:lighting():set_ambient_light(.15, .15, .15, 1)
    nngn:lighting():set_shadows_enabled(true)
    player.move_all(0, 0, true)
    nngn:camera():set_pos(0, -128, 32)
end

local function check_collectable(e)
    local p = deref(e)
    if not collectables[p] then return false end
    collectables[p] = nil
    for i, v in ipairs(entities) do
        if v == e then
            entities[i] = entities[#entities]
            entities[#entities] = nil
            break
        end
    end
    nngn:remove_entity(e)
    return true
end

local function on_collision(e0, e1)
    if e0 == warp or e1 == warp then
        map.next("maps/main.lua")
        return
    end
    if check_collectable(e0) or check_collectable(e1) then
        return
    end
    local d0, d1 <const> = deref(e0), deref(e1)
    if switches[d0] or switches[d1] then
        on_switch = true
        return
    end
    if not anim and (e0 == anim_switch or e1 == anim_switch) then
        local r, a <const> = math.pi / 2, math.pi / 8
        nngn:camera():set_rot(r, 0, 0)
        anim = animation.chain{
            animation.camera({0, 0, 32}, {0, 4 * camera.MAX_VEL, 0}),
            animation.camera_rot({r, 0, -2 * math.pi}, {0, 0, -a}),
            animation.camera({0, -96, 32}, {0, -4 * camera.MAX_VEL, 0}),
        }
    end
end

local function heartbeat()
    if anim then
        anim = anim:update(nngn:timing():dt_ms())
    end
    if on_switch then
        door:set_vel(64, 0, 0)
    else
        door:set_vel(-64, 0, 0)
    end
    on_switch = false
    local new_p
    local p = {door:pos()}
    local v = {door:vel()}
    if v[1] < 0 and p[1] <= 0 then
        new_p = 0
    elseif v[1] > 0 and p[1] >= 64 then
        new_p = 64
    end
    if new_p then
        nngn.entities:set_pos(door, table.unpack(p))
        door:set_vel(0, 0, 0)
    end
end

map.map {
    name = "wolfenstein",
    file = "maps/wolfenstein.lua",
    state = {
        camera_follow = true, perspective = true,
        ambient_light = true, shadows_enabled = true,
    },
    init = init,
    entities = entities,
    heartbeat = heartbeat,
    on_collision = on_collision,
    reset = function() camera.reset() end,
    tiles = {tex.tex, 8, 0, 16, 192, 288, 1, 1, {0, 0}},
}
