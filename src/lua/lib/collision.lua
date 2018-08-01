local camera <const> = require "nngn.lib.camera"

local function default_backends()
    return {
        {CollisionBackend.native, {}}}
end

local function init(backends)
    if nngn:colliders():has_backend() then return end
    backends = backends or default_backends()
    for _, t in ipairs(backends) do
        local f, i = table.unpack(t)
        local b = f(table.unpack(i))
        if b and nngn:colliders():set_backend(b) then return end
    end
end

local function set_max_colliders(n)
    nngn:colliders():set_max_colliders(n)
    nngn:renderers():set_max_colliders(n)
end

local function set_resolve(b)
    nngn:colliders():set_resolve(b)
    camera.get():set_ignore_limits(not b)
end

local function toggle_resolve()
    set_resolve(not nngn:colliders():resolve())
end

return {
    init = init,
    set_max_colliders = set_max_colliders,
    set_resolve = set_resolve,
    toggle_resolve = toggle_resolve,
}
