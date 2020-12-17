local input = require "nngn.lib.input"

local BACK_ENDS <const> = {
    vulkan = {
        Graphics.VULKAN_BACKEND,
        Graphics.vulkan_params{debug = Platform.debug},
    },
    opengl_es = {
        Graphics.OPENGL_ES_BACKEND,
        Graphics.opengl_params{maj = 3, min = 1, debug = Platform.debug},
    },
    opengl = {
        Graphics.OPENGL_BACKEND,
        Graphics.opengl_params{maj = 4, min = 2, debug = Platform.debug},
    },
    terminal = {
        Graphics.TERMINAL_BACKEND,
        Graphics.terminal_params{
            flags =
                Graphics.TERMINAL_FLAG_REPOSITION
                | Graphics.TERMINAL_FLAG_HIDE_CURSOR,
        },
    },
    pseudograph = {Graphics.PSEUDOGRAPH},
}

local BACK_END_LIST <const> = {
    BACK_ENDS.vulkan,
    BACK_ENDS.opengl_es,
    BACK_ENDS.opengl,
    BACK_ENDS.terminal,
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
    VULKAN = BACK_ENDS.vulkan,
    OPENGL_ES = BACK_ENDS.opengl_es,
    OPENGL = BACK_ENDS.opengl,
    TERMINAL = BACK_ENDS.terminal,
    PSEUDOGRAPH = BACK_ENDS.pseudograph,
    init = init,
}
