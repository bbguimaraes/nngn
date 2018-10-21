local input = require "nngn.lib.input"

local function default_backends()
    local debug = Platform.DEBUG
    return {{
        Graphics.OPENGL_ES_BACKEND,
        Graphics.opengl_params{maj = 3, min = 1, debug = debug},
    }, {
        Graphics.OPENGL_BACKEND,
        Graphics.opengl_params{maj = 4, min = 2, debug = debug},
    }, {Graphics.PSEUDOGRAPH}}
end

local function init(backends)
    if nngn.graphics then return end
    backends = backends or default_backends()
    for _, x in ipairs(backends) do
        local b, args = table.unpack(x)
        if nngn:set_graphics(b, args) then return end
    end
    error("failed to initialize graphics back end")
end

return {
    init = init,
}
