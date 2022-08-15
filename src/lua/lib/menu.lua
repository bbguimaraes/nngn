local entity <const> = require "nngn.lib.entity"
local utils <const> = require "nngn.lib.utils"

local MENU <const> = {
    idx = 1,
    actions = {true},
}

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

local function show_menu(m)
    local idx = m.idx
    local n <const> = #m.actions
    local entities <const> = {}
    local actions <const> = {}
    local box <const> = {
        renderer = {
            type = Renderer.SCREEN_SPRITE,
            tex = "img/zelda/zelda.png", size = {96, 96},
            scale = {512//8, 512//8}, coords = {18, 0, 21, 3},
        },
    }
    for i, t in ipairs{true} do
        local pos <const> = {menu_pos(n, idx, i)}
        box.pos = pos
        table.insert(entities, entity.load(nil, nil, box))
        table.insert(actions, entity.load(nil, nil, {
            pos = pos,
        }))
    end
    m.entities = entities
    m.action_icons = actions
end

local function hide_menu(m)
    for _, x in ipairs(m.entities) do
        nngn:remove_entity(x)
    end
    for _, x in ipairs(m.action_icons) do
        nngn:remove_entity(x)
    end
    m.entities = nil
    m.action_icons = nil
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

local function menu(mods)
    local m <const> = MENU
    if displayed(m) then
        rotate_menu(m, mods)
    else
        show_menu(m)
    end
end

local function action()
    local m <const> = MENU
    if displayed(m) then
        hide_menu(m)
    end
end

return {
    menu = menu,
    action = action,
    displayed = displayed,
    show = show_menu,
    hide = hide_menu,
}
