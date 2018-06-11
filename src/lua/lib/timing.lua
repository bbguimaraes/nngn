local TIME_SCALES = {0.1, 0.2, 0.5, 1, 2, 5, 10}

local function scales() return TIME_SCALES end
local function set_scales(t) TIME_SCALES = t end

return {
    scales = scales,
    set_scales = set_scales,
}
