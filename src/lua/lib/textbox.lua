local SPEED_NORMAL = Textbox.DEFAULT_SPEED
local SPEED_FAST = SPEED_NORMAL // 4

local function set_speed_normal()
    nngn:textbox():set_speed(SPEED_NORMAL)
end

local function set_speed_fast()
    nngn:textbox():set_speed(SPEED_FAST)
end

local function set(title, text)
    local t <const> = nngn:textbox()
    if title then
        t:set_title(title)
    end
    if text then
        t:set_text(text)
    end
end

local function update(title, text)
    local t <const> = nngn:textbox()
    if title and title ~= t:title() then
        t:set_title(title)
    end
    if text and text ~= t:text() then
        t:set_text(text)
    end
end

local function clear() set("", "") end

return {
    SPEED_NORMAL = SPEED_NORMAL,
    SPEED_FAST = SPEED_FAST,
    set_speed_normal = set_speed_normal,
    set_speed_fast = set_speed_fast,
    set = set,
    update = update,
    clear = clear,
}
