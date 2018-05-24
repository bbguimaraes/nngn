local input = require "nngn.lib.input"

local BACK_ENDS <const> = {
    glfw = {Graphics.GLFW_BACKEND},
    pseudograph = {Graphics.PSEUDOGRAPH},
}

local BACK_END_LIST <const> = {
    BACK_ENDS.glfw,
    BACK_ENDS.pseudograph,
}

local function init(backends)
    if nngn:graphics() then
        return
    end
    for _, t in ipairs(backends or BACK_END_LIST) do
        if type(t) == "string" then
            t = BACK_ENDS[t]
        end
        if nngn:set_graphics(table.unpack(t)) then
            return
        end
    end
    error("failed to initialize graphics back end")
end

return {
    GLFW = BACK_ENDS.glfw,
    PSEUDOGRAPH = BACK_ENDS.pseudograph,
    init = init,
}
