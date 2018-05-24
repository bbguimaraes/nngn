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
    {"I", Input.SEL_PRESS | Input.SEL_CTRL, function(_, _, mods)
        local m = 1
        if mods & Input.MOD_SHIFT ~= 0 then
            m = -1
        end
        nngn:graphics():set_swap_interval(nngn:graphics():swap_interval() + m)
    end},
}, ctrl_input)

register({
    {"P", Input.SEL_PRESS, resume},
}, paused_input)

register({
    {"P", Input.SEL_PRESS, pause},
}, input)

local function install()
    nngn:input():set_binding_group(input)
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
