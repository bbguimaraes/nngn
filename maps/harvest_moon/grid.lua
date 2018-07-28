local player <const> = require "nngn.lib.player"

local vec2 = require("nngn.lib.math").vec2

local SIZE <const> = 16

local function round(x)
    if x < 0 then
        return -math.ceil(-x)
    else
        return math.floor(x)
    end
end

local grid <const> = {
    SIZE = SIZE,
}
grid.__index = grid

function grid:new()
    return setmetatable({
        size = {36, 26},
        offset = {192, 112},
    }, self)
end

function grid.abs_pos(x, y)
    return SIZE * (x + 0.5), SIZE * (y + 0.5)
end

function grid.pos(x, y)
    return round(x / SIZE), round(y / SIZE)
end

function grid:get_target(p)
    local e <const> = p.entity
    return self.pos(table.unpack(
        vec2(e:pos())
            + player.face_vec(p, 1.125 * SIZE)
            + vec2(0, 0.5 * e:renderer():z_off())))
end

return grid
