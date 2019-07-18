local function default_backends()
    local debug = Platform.DEBUG
    return {
        {Compute.OPENCL_BACKEND, Compute.opencl_params{debug = debug}},
        {Compute.PSEUDOCOMP}}
end

local function init(backends)
    if nngn:compute() then return end
    backends = backends or default_backends()
    for _, x in ipairs(backends) do
        local b, args = table.unpack(x)
        nngn:set_compute(b, args)
        if nngn:compute() then return end
    end
end

return {
    init = init,
}
