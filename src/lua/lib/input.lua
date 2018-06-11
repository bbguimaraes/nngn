local camera = require "nngn.lib.camera"
local nngn_math = require "nngn.lib.math"
local player = require "nngn.lib.player"
local utils = require "nngn.lib.utils"

local input = BindingGroup.new()
local paused_input = BindingGroup.new()

local function to_byte(k)
    if type(k) == "number" then return k end
    return string.byte(k)
end

local function key_event(key, action, mod)
    nngn.input:key_callback(to_byte(key), action, mod or 0)
end

local function pause() nngn.input:set_binding_group(paused_input) end
local function resume() nngn.input:set_binding_group(input) end
local function press(key, mod) key_event(key, Input.KEY_PRESS, mod) end
local function release(key, mod) key_event(key, Input.KEY_RELEASE, mod) end

local function register(t, ...)
    local inputs = {...}
    for _, t in ipairs(t) do
        for _, g in ipairs(inputs) do
            t[1] = to_byte(t[1])
            g:add(table.unpack(t))
        end
    end
end

register({
    {Input.KEY_ESC, Input.SEL_PRESS, function() nngn:exit() end},
    {"B", Input.SEL_PRESS | Input.SEL_CTRL, function(_, _, mods)
        nngn.renderers:set_debug(utils.shift(
            nngn.renderers:debug(), math.tointeger(2 ^ Renderers.N_DEBUG),
            mods & Input.MOD_SHIFT == 0))
    end},
    {"I", Input.SEL_PRESS | Input.SEL_CTRL, function(_, _, mods)
        local m = 1
        if mods & Input.MOD_SHIFT ~= 0 then m = -1 end
        nngn.graphics:set_swap_interval(nngn.graphics:swap_interval() + m)
    end},
}, input, paused_input)

register({
    {"P", Input.SEL_PRESS, resume},
}, paused_input)

register({
    {"A", 0, player.move},
    {"D", 0, player.move},
    {"S", 0, player.move},
    {"W", 0, player.move},
    {"C", Input.SEL_PRESS, function(_, _, mods)
        if mods & Input.MOD_CTRL == 0 then return camera.toggle_follow() end
        if mods & Input.MOD_ALT == 0 then return camera.reset() end
        local p = not nngn.camera:perspective()
        nngn.camera:set_perspective(p)
        nngn.renderers:set_perspective(p)
    end},
    {"P", Input.SEL_PRESS, function(_, _, mods)
        if mods & Input.MOD_CTRL == 0 then
            pause()
        elseif mods & Input.MOD_SHIFT == 0 then
            local e = nngn.entities:add()
            player.load(e)
            nngn.players:add(e)
            if nngn.players:n() == 1 then camera.set_follow(e) end
        else
            local p = nngn.players:cur()
            if p then player.remove(p) end
        end
    end},
    {Input.KEY_TAB, Input.SEL_PRESS, function(_, _, mods)
        if mods & Input.MOD_SHIFT == 0 then player.next(1)
        else player.next(-1) end
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
    local pos = nngn_math.vec2()
    nngn.mouse_input:register_button_callback(function(button, press)
        if button ~= 1 then return end
        local m
        if press then
            m = Graphics.CURSOR_MODE_DISABLED
            pressed_button = button
        else
            m = Graphics.CURSOR_MODE_NORMAL
            pressed_button = nil
        end
        nngn.graphics:set_cursor_mode(m)
    end)
    nngn.mouse_input:register_move_callback(function(x, y)
        if pressed_button == 1 then
            local r = nngn_math.vec3(nngn.camera:rot())
            local r2 = nngn_math.vec2(0x1p-7) * (nngn_math.vec2(x, y) - pos)
            r[1] = r[1] - r2[2]
            r[3] = r[3] - r2[1]
            nngn.camera:set_rot(table.unpack(r))
        end
        pos[1], pos[2] = x, y
    end)
end

local function install()
    resume()
    register_mouse()
end

return {
    input = input,
    paused_input = paused_input,
    pause = pause,
    resume = resume,
    press = press,
    release = release,
    install = install,
}
