local animation = require "nngn.lib.animation"
local camera = require "nngn.lib.camera"
local entity = require "nngn.lib.entity"
local input = require "nngn.lib.input"
local map = require "nngn.lib.map"
local player = require "nngn.lib.player"
local textbox = require "nngn.lib.textbox"
local vec3 = require("nngn.lib.math").vec3

local entities, animations = {}, {}
local sword_bb = {-5, -16, 11, -10}
local warp, sign, chest, sword

local function init()
    local coll_ts = Collider.TRIGGER | Collider.SOLID
    -- warp
    table.insert(entities, entity.load(nil, nil, {
        pos = {8, -260}, collider = {
            type = Collider.AABB, bb = {-16, -4, 16, 4},
            flags = Collider.TRIGGER}}))
    warp = entities[#entities]
    -- sign
    table.insert(entities, entity.load(nil, nil, {
        pos = {-8, 92}, collider = {
            type = Collider.AABB,
            bb = {-8, -4, 8, 4}, m = Math.INFINITY, flags = coll_ts}}))
    sign = entities[#entities]
    -- chest
    do
        local t = dofile("src/lson/chest.lua")
        t.pos = {-32, 192, 0}
        t.collider.m = Math.INFINITY
        t.collider.flags = Collider.TRIGGER | Collider.SOLID
        table.insert(entities, entity.load(nil, nil, t))
    end
    chest = entities[#entities]
    -- sword
    table.insert(entities, entity.load(nil, nil, {
        pos = {-11, 117},
        renderer = {
            type = Renderer.SPRITE,
            tex = "img/zelda.png",
            size = {16, 32}, scale = {512//16, 512//32}, coords = {5, 0}},
        collider = {
            type = Collider.AABB, bb = sword_bb, m = Math.INFINITY,
            flags = Collider.TRIGGER | Collider.SOLID}}))
    sword = entities[#entities]
    for _, t in pairs({
        -- walls
        {{   0,  240}, {-112,  -16, 112,  16},   0.0},
        {{-128,  -12}, { -16, -236,  16, 236},   0.0},
        {{ 128,  -12}, { -16, -236,  16, 236},   0.0},
        {{ -64, -264}, { -48,  -16,  48,  16},   0.0},
        {{  72, -264}, { -40,  -16,  40,  16},   0.0},
        -- trunk
        {{ -12, -242}, {  -4,  -14,   4,  14},   0.0},
        {{  28, -242}, {  -4,  -14,   4,  14},   0.0},
        -- platform
        {{  -8,  152}, { -40,   -8,  40,   8},   0.0},
        {{ -72,  100}, {  -8,  -28,   8,  28},   0.0},
        {{  56,  100}, {  -8,  -28,   8,  28},   0.0},
        {{ -60,  140}, {  -5,  -23,   5,  23}, 315.0},
        {{  44,  140}, {  -5,  -23,   5,  23},  45.0},
        {{ -60,   60}, {  -6,  -22,   6,  22},  45.0},
        {{  44,   60}, {  -6,  -22,   6,  22}, 315.0},
        -- gate
        {{ -44,   48}, {  -4,   -8,   4,   8},   0.0},
        {{  28,   48}, {  -4,   -8,   4,   8},   0.0},
        -- altar
        {{ -20,  104}, {  -4,  -16,   4,  16},   0.0},
        {{   4,  104}, {  -4,  -16,   4,  16},   0.0},
    }) do
        if t[3] ~= 0.0 then t[3] = math.rad(t[3]) end
        table.insert(entities, entity.load(nil, nil, {
            pos = t[1], collider = {
                type = Collider.BB,
                bb = t[2], rot = t[3], m = Math.INFINITY,
                flags = Collider.SOLID}}))
    end
    -- tree tops
    table.insert(entities, entity.load(nil, nil, {
        pos = {0, 0}, z_off = -128,
        renderer = {
            type = Renderer.SPRITE,
            tex = "img/zelda_sacred_grove.png",
            size = {256, 512}, scale = {512//256, 1}, coords = {1, 0}}}))
    player.move_all(8, -252, true)
end

local function heartbeat()
    local dt = nngn.timing:dt_ms()
    for i, a in pairs(animations) do
        animations[i] = a:update(dt)
    end
end

local function on_collision(e0, e1)
    if e0 == warp or e1 == warp then
        map.next("maps/main.lua")
    elseif e0 == sign or e1 == sign then
        textbox.update(
            "An unnamed old sign",
            "IT'S DANGEROUS TO GO\nALONE! TAKE THIS.")
    elseif e0 == chest or e1 == chest then
        do
            local t = dofile("src/lson/chest.lua")
            local r, c = t.renderer, t.collider
            r.coords[1] = r.coords[1] + 1
            c.flags = Collider.SOLID
            c.m = Math.INFINITY
            entity.load(chest, nil, {renderer = r, collider = c})
        end
        local pos, half = vec3(chest:pos()), vec3(0.5, 0.5, 0)
        local t = dofile("src/lson/fairy1.lua")
        t.max_vel = player.MAX_VEL
        t.collider = nil
        local fairies = {}
        for _ = 1, 16 do
            t.pos = pos
                + vec3(16)
                * (vec3(math.random(), math.random(), 0) - half)
            local e = entity.load(nil, nil, t)
            table.insert(entities, e)
            table.insert(fairies, e)
        end
        table.insert(animations, animation.flock(
            player.entity(),
            fairies, 8 * player.MAX_VEL, 16))
    elseif e0 == sword or e1 == sword then
        nngn.renderers:remove(sword:renderer())
        sword:set_renderer(nil)
        entity.load(sword, nil, {
            collider = {
                type = Collider.AABB,
                bb = sword_bb, m = Math.INFINITY,
                flags = Collider.SOLID}})
        local anim_state = {camera = true, camera_follow = true}
        table.insert(animations, animation.chain{
            animation.pause_input(),
            animation.save_state(anim_state),
            animation.stop(),
            animation.fn(function()
                nngn.lighting:set_zsprites(true)
                nngn.renderers:set_zsprites(true)
                nngn.lighting:set_shadows_enabled(true)
                nngn.lighting:set_ambient_anim{
                    rate_ms = 0, f = {
                        type = AnimationFunction.LINEAR,
                        v = 1, step_s = -1, ["end"] = .3}}
                player.fairy(nil, true)
                return true
            end),
            animation.textbox(
                "Fairy", "Watch as I conjure up a random cutscene."),
            animation.fn(function()
                local fairies = {}
                for _, t in ipairs{
                    {"src/lson/fairy0.lua", {-96,  0, 0}},
                    {"src/lson/fairy1.lua", {-32, 48, 0}},
                } do
                    local f, pos, anim = table.unpack(t)
                    local t = dofile(f)
                    t.pos = pos
                    t.collider.flags = Collider.SOLID
                    local e = entity.load(nil, nil, t)
                    table.insert(fairies, e)
                    table.insert(entities, e)
                end
                local e0, e1 = table.unpack(fairies)
                local v = player.MAX_VEL
                table.insert(animations, animation.cycle{
                    animation.velocity(e0, { 32, 0, 0}, { v, 0, 0}),
                    animation.velocity(e0, {-48, 0, 0}, {-v, 0, 0}),
                })
                table.insert(animations, animation.cycle{
                    animation.sprite(e1, player.ANIMATION.FRIGHT),
                    animation.velocity(e1, { 12, 0, 0}, { v/2, 0, 0}),
                    animation.timer(1000),
                    animation.sprite(e1, player.ANIMATION.FLEFT),
                    animation.velocity(e1, {-28, 0, 0}, {-v/2, 0, 0}),
                    animation.timer(1000),
                })
                return true
            end),
            animation.timer(1000),
            animation.camera({-8, -128, 0}, {0, -camera.MAX_VEL, 0}),
            animation.textbox(
                "The sacred grove",
                Textbox.TEXT_RED_STR .. "The "
                    .. Textbox.TEXT_GREEN_STR .. "legend "
                    .. Textbox.TEXT_BLUE_STR .. "of "
                    .. Textbox.TEXT_WHITE_STR .. "Zelda"),
            animation.restore_state(anim_state),
            animation.resume_input(),
        })
    end
end

map.map {
    name = "zelda",
    file = "maps/zelda.lua",
    state = {ambient_light = true, zsprites = true, shadows_enabled = true},
    init = init,
    entities = entities,
    heartbeat = heartbeat,
    on_collision = on_collision,
    reset = function() nngn.lighting:set_ambient_anim{} end,
    tiles = {
        "img/zelda_sacred_grove.png",
        2, 0, 0, 256, 256, 1, 2, {0, 0, 0, 1},
    },
}
