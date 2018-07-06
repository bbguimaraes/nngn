local function default_backends()
    return {{Graphics.GLFW_BACKEND}, {Graphics.PSEUDOGRAPH}}
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
