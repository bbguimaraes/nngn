local SPEED_NORMAL = Textbox.DEFAULT_SPEED
local SPEED_FAST = SPEED_NORMAL // 4

local function set_speed_normal()
    nngn.textbox:set_speed(SPEED_NORMAL)
end

local function set_speed_fast()
    nngn.textbox:set_speed(SPEED_FAST)
end

local function update(title, str)
    if title then nngn.textbox:set_title(title) end
    if str then nngn.textbox:set_text(str) end
end

local function clear() update("", "") end

return {
    SPEED_NORMAL = SPEED_NORMAL,
    SPEED_FAST = SPEED_FAST,
    set_speed_normal = set_speed_normal,
    set_speed_fast = set_speed_fast,
    update = update,
    clear = clear,
}
