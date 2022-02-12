local utils <const> = require "nngn.lib.utils"

local kernel <const> = utils.class "kernel" {}

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

function kernel:new(...)
    return setmetatable({{...}}, self)
end

function kernel:__bor(t)
    return table.move(t, 1, #t, #self + 1, self)
end

function kernel:execute(wait, events)
    local k0 = self[1]
    table.insert(k0, wait)
    table.insert(k0, events)
    if not nngn.compute:execute(table.unpack(k0)) then
        return false
    end
    if #self == 1 then
        return true
    end
    for i = 2, #self do
        local k1 <const> = self[i]
        table.insert(k1, {events[#events]})
        table.insert(k1, events)
        if not nngn.compute:execute(table.unpack(k1)) then
            return false
        end
        k0 = k1
    end
    return true
end

return {
    init = init,
    kernel = kernel,
}
