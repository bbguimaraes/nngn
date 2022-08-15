local entity <const> = require "nngn.lib.entity"
local utils <const> = require "nngn.lib.utils"

local function displayed(m) return m.entities ~= nil end

local function menu_pos(n, idx, i)
    local x = (n - idx + i) % n + 2
    if idx == i then
        x = x - 1
    else
        x = x - 0.5
    end
    return 96 * x, 64, 0
end

local function show_menu(p, m)
    local idx = m.idx
    local n <const> = #m.actions
    local entities <const> = {}
    local actions <const> = {}
    local fairy <const> = dofile("src/lson/zelda/fairy2.lua").renderer
    local fire <const> = dofile("src/lson/zelda/fire.lua").renderer
    fairy.size = {64, 64}
    fire.size = {64, 64}
    local box <const> = {
        renderer = {
            type = Renderer.SCREEN_SPRITE,
            tex = "img/zelda/zelda.png", size = {96, 96},
            scale = {512//8, 512//8}, coords = {18, 0, 21, 3},
        },
    }
    for i, t in ipairs{
        {
            tex = "img/chrono_trigger/cathedral.png", size = {32, 64},
            scale = {512//8, 512//16}, coords = {44, 14},
        },
        {
            tex = "img/zelda/zelda.png", size = {64, 64},
            scale = {512//16, 512//16}, coords = {4, 1},
        },
        fairy,
        fire,
        {
            tex = "img/zelda/zelda.png", size = {64, 64},
            scale = {512//16, 512//16}, coords = {12, 0},
        },
        {
            tex = "img/zelda/zelda.png", size = {64, 64},
            scale = {512//16, 512//16}, coords = {16, 0},
        },
    } do
        local pos <const> = {menu_pos(n, idx, i)}
        t.type = Renderer.SCREEN_SPRITE
        box.pos = pos
        table.insert(entities, entity.load(nil, nil, box))
        table.insert(actions, entity.load(nil, nil, {
            pos = pos,
            renderer = t,
        }))
    end
    m.entities = entities
    m.action_icons = actions
    m.bar = entity.load(nil, nil, {
        pos = {96, 256},
        renderer = {
            type = Renderer.SCREEN_SPRITE,
            tex = "img/zelda/zelda.png", size = {64, 192},
            scale = {512//16, 512//16}, coords = {14, 4, 15, 7},
        },
    })
    local meter <const> = {}
    local mana <const> = p.data.mana
    for i = 0, mana - 1 do
        table.insert(meter, entity.load(nil, nil, {
            pos = {96, 184 + 8 * #meter, 1},
            renderer = {
                type = Renderer.SCREEN_SPRITE,
                tex = "img/zelda/zelda.png",
                size = {32, 8},
                scale = {512//8, 512//2},
                coords = {10, 14 + ((i == mana - 1) and 1 or 0)},
            },
        }))
    end
    m.meter = meter
end

local function hide_menu(m)
    for _, x in ipairs(m.entities) do
        nngn:remove_entity(x)
    end
    for _, x in ipairs(m.action_icons) do
        nngn:remove_entity(x)
    end
    for _, x in ipairs(m.meter) do
        nngn:remove_entity(x)
    end
    nngn:remove_entity(m.bar)
    m.entities = nil
    m.action_icons = nil
    m.meter = nil
    m.bar = nil
end

local function rotate_menu(m, mods)
    local n <const> = #m.actions
    local idx = m.idx
    idx = utils.shift(idx, n, (mods & Input.MOD_SHIFT) == 0, 1)
    m.idx = idx
    for i, x in ipairs(m.action_icons) do
        x:set_pos(menu_pos(n, idx, i))
    end
end

local function menu(p, mods)
    local m <const> = p.data.menu
    if displayed(m) then
        rotate_menu(m, mods)
    else
        show_menu(p, m)
    end
end

local function new_player()
    local player <const> = require "nngn.lib.player"
    return {
        idx = 1,
        actions = {
            function(p, press) if press then player.light(p) end end,
            function(p, press) if press then player.flashlight(p) end end,
            function(p, press) if press then player.fairy(p) end end,
            player.fire,
            function(p, press) if press then player.sword(p) end end,
            function(p, press)
                if press then
                    local m <const> = p.data.menu
                    player.mana(p, 1)
                    menu(p)
                end
            end,
        },
    }
end

return {
    new_player = new_player,
    menu = menu,
    displayed = displayed,
    show = show_menu,
    hide = hide_menu,
}
