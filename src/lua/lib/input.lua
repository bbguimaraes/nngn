local camera = require "nngn.lib.camera"
local collision = require "nngn.lib.collision"
local light = require "nngn.lib.light"
local nngn_math = require "nngn.lib.math"
local menu = require "nngn.lib.menu"
local player = require "nngn.lib.player"
local textbox = require "nngn.lib.textbox"
local timing = require "nngn.lib.timing"
local utils = require "nngn.lib.utils"

local input <const> = BindingGroup.new()
local ctrl_input <const> = BindingGroup.new()
local paused_input <const> = BindingGroup.new()
local paused_prev_input

input:set_next(ctrl_input)
paused_input:set_next(ctrl_input)

local function to_byte(k)
    if type(k) == "number" then return k end
    return string.byte(k)
end

local function key_event(key, action, mod)
    nngn:input():key_callback(to_byte(key), action, mod or 0)
end

local function pause()
    local i <const> = nngn:input()
    paused_prev_input = i:binding_group()
    i:set_binding_group(paused_input)
end

local function resume() nngn:input():set_binding_group(paused_prev_input) end
local function press(key, mod) key_event(key, Input.KEY_PRESS, mod) end
local function release(key, mod) key_event(key, Input.KEY_RELEASE, mod) end

local function register(t, g)
    for _, t in ipairs(t) do
        t[1] = to_byte(t[1])
        g:add(table.unpack(t))
    end
end

register({
    {Input.KEY_ESC, Input.SEL_PRESS, function() nngn:exit() end},
    {"B", Input.SEL_PRESS | Input.SEL_CTRL, function(_, _, mods)
        nngn:renderers():set_debug(utils.shift(
            nngn:renderers():debug(), Renderers.DEBUG_ALL + 1,
            mods & Input.MOD_SHIFT == 0))
    end},
    {"G", Input.SEL_PRESS | Input.SEL_CTRL, function()
        nngn:grid():set_enabled(not nngn:grid():enabled())
    end},
    {"I", Input.SEL_PRESS | Input.SEL_CTRL, function(_, _, mods)
        local m = 1
        if mods & Input.MOD_SHIFT ~= 0 then
            m = -1
        end
        nngn:graphics():set_swap_interval(nngn:graphics():swap_interval() + m)
    end},
    {"L", Input.SEL_PRESS | Input.SEL_CTRL, function()
        nngn:lighting():set_enabled(not nngn:lighting():enabled())
    end},
    {"M", Input.SEL_PRESS | Input.SEL_CTRL, function()
        nngn:map():set_enabled(not nngn:map():enabled())
    end},
    {"V", Input.SEL_PRESS | Input.SEL_CTRL, function(_, _, mods)
        nngn:timing():set_scale(utils.shift_table(
            nngn:timing():scale(), timing.scales(), 1,
            mods & Input.MOD_SHIFT == 0, nngn_math.float_eq))
    end},
    {"Z", Input.SEL_PRESS | Input.SEL_CTRL, function(_, _, mods)
        local z <const> = not nngn:renderers():zsprites()
        nngn:renderers():set_zsprites(z)
        nngn:lighting():set_zsprites(z)
    end},
    {" ", 0, function(_, press)
        if not press then
            textbox.set_speed_normal()
        elseif not nngn:textbox():finished() then
            textbox.set_speed_fast()
        else
            textbox.clear()
        end
    end},
}, ctrl_input)

register({
    {"P", Input.SEL_PRESS, resume},
    {"S", Input.SEL_PRESS | Input.SEL_CTRL, function(key, press, mods)
        nngn:lighting():set_shadows_enabled(
            not nngn:lighting():shadows_enabled())
    end},
}, paused_input)

register({
    {"A", 0, player.move},
    {"D", 0, player.move},
    {"S", 0, function(key, press, mods)
        if mods & Input.MOD_CTRL == 0 then
            player.move(key, press, mods)
        elseif press then
            nngn:lighting():set_shadows_enabled(
                not nngn:lighting():shadows_enabled())
        end
    end},
    {"W", 0, player.move},
    {"C", Input.SEL_PRESS, function(_, _, mods)
        if mods & Input.MOD_CTRL == 0 then
            camera.toggle_follow()
        elseif mods & Input.MOD_ALT == 0 then
            camera.reset()
        else
            camera.toggle_perspective()
        end
    end},
    {"E", Input.SEL_PRESS, function(_, _, mods) player.menu(nil, mods) end},
    {"F", 0, function(_, press) player.action(nil, press) end},
    {"N", Input.SEL_PRESS | Input.SEL_CTRL, function(_, _, mods)
        if mods & Input.MOD_ALT ~= 0 then return light.sun() end
        local a = {nngn:lighting():ambient_light()}
        if mods & Input.MOD_SHIFT == 0
        then for i = 1, 3 do a[i] = math.min(a[i] * 2, 1) end
        else for i = 1, 3 do a[i] = math.max(a[i] / 2, 0) end end
        nngn:lighting():set_ambient_light(table.unpack(a))
    end},
    {"O", Input.SEL_PRESS | Input.SEL_CTRL, function()
        collision:toggle_resolve()
    end},
    {"P", Input.SEL_PRESS, function(_, _, mods)
        if mods & Input.MOD_CTRL == 0 then
            return pause()
        end
        local shift <const> = mods & Input.MOD_SHIFT ~= 0
        local alt <const> = mods & Input.MOD_ALT ~= 0
        if shift and not alt then
            player.remove()
        elseif alt then
            player.load(nil, not shift)
        else
            player.add()
        end
    end},
    {"T", Input.SEL_PRESS, function()
        textbox.set("test",
            Textbox.TEXT_RED_STR .. "t"
            .. Textbox.TEXT_GREEN_STR .. "e"
            .. Textbox.TEXT_BLUE_STR .. "s"
            .. Textbox.TEXT_WHITE_STR .. "t")
    end},
    {Input.KEY_TAB, 0, function(_, press, mods)
        if not press then
            return nngn:renderers():remove_selection(player.entity():renderer())
        end
        if mods & Input.MOD_SHIFT == 0 then
            player.next(1, press)
        else
            player.next(-1, press)
        end
    end},
    {Input.KEY_LEFT, 0, camera.move},
    {Input.KEY_RIGHT, 0, camera.move},
    {Input.KEY_DOWN, 0, camera.move},
    {Input.KEY_UP, 0, camera.move},
    {Input.KEY_PAGE_DOWN, 0, camera.move},
    {Input.KEY_PAGE_UP, 0, camera.move},
}, input)

local function register_mouse()
    local pressed_button
    local pos_x, pos_y = 0, 0
    local ignore = false
    nngn:mouse_input():register_button_callback(function(button, press)
        if button ~= 1 then return end
        local m
        if press then
            m = Graphics.CURSOR_MODE_DISABLED
            pressed_button = button
        else
            m = Graphics.CURSOR_MODE_NORMAL
            ignore = true
            pressed_button = nil
        end
        nngn:graphics():set_cursor_mode(m)
    end)
    nngn:mouse_input():register_move_callback(function(x, y)
        if ignore then
            ignore = false
        elseif pressed_button == 1 then
            local c <const> = camera:get()
            local sx <const>, sy <const> = c:screen()
            camera.rotate((pos_x - x) / sx, (pos_y - y) / sy, 0)
        end
        pos_x, pos_y = x, y
    end)
end

local function install()
    nngn:input():set_binding_group(input)
    register_mouse()
end

return {
    input = input,
    paused_input = paused_input,
    ctrl_input = ctrl_input,
    pause = pause,
    resume = resume,
    press = press,
    release = release,
    install = install,
}
