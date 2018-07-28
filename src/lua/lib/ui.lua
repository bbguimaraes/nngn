local nngn_math <const> = require "nngn.lib.math"
local utils <const> = require "nngn.lib.utils"

local WHEEL_TIMER <const> = 250

local wheel = utils.class "wheel" {}

function wheel:new(n, radius)
    return setmetatable({
        n = n,
        pos = 0,
        radius = radius,
        timer = WHEEL_TIMER,
        dir = 1,
    }, self)
end

function wheel:cycle(dir)
    self.pos = (self.pos + self.n + dir) % self.n
    self.timer = 0
    self.dir = dir
end

function wheel:dismiss()
    self.timer = WHEEL_TIMER
end

function wheel:update(dt, f)
    local timer = self.timer
    if timer < WHEEL_TIMER then
        timer = math.min(timer + dt, WHEEL_TIMER)
        self.timer = timer
    end
    local n <const>, pos <const>, dir <const> = self.n, self.pos, self.dir
    local step <const> = 2 * math.pi / n
    local initial <const> = math.pi / 2 + (pos - dir) * step
    local t <const> = math.sqrt(timer / WHEEL_TIMER)
    local a <const> = initial + dir * t * step
    for i, x, y in nngn_math.circle_iter(self.radius, a, n) do
        f(i, x, y)
    end
end

return {
    wheel = wheel,
}
