local function default_backends()
    local t = {
        {CollisionBackend.native, {}}}
    if nngn.compute then
        table.insert(t, 1, {CollisionBackend.compute, {nngn.compute}})
    end
    return t
end

local function init(backends)
    if nngn.colliders:has_backend() then return end
    backends = backends or default_backends()
    for _, t in ipairs(backends) do
        local f, i = table.unpack(t)
        local b = f(table.unpack(i))
        if b and nngn.colliders:set_backend(b) then return end
    end
end

return {
    init = init,
}
