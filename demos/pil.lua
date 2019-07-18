dofile "src/lua/path.lua"
local entity <const> = require "nngn.lib.entity"

local SIZE <const> = Textures.SIZE
local EXTENT <const> = Textures.EXTENT

require("nngn.lib.graphics").init()
require("nngn.lib.input").install()

nngn:graphics():resize_textures(2)
nngn:textures():set_max(2)
nngn:entities():set_max(1)
nngn:renderers():set_max_sprites(1)

local function disk(cx, cy, r)
    return function(x, y)
        return (x - cx) ^ 2 + (y - cy) ^ 2 <= r ^ 2
    end
end

local function rect(left, right, bottom, up)
    return function(x, y)
        return left <= x and x <= right
            and bottom <= y and y <= up
    end
end

local function complement(r)
    return function(x, y)
        return not r(x, y)
    end
end

local function union(r1, r2)
    return function(x, y)
        return r1(x, y) or r2(x, y)
    end
end

local function intersection(r1, r2)
    return function(x, y)
        return r1(x, y) and r2(x, y)
    end
end

local function difference(r1, r2)
    return intersection(r1, complement(r2))
end

local function translate(r, dx, dy)
    return function(x, y)
        return r(x - dx, y - dy)
    end
end

local function plot(t, v, r)
    local w <const>, h <const> = EXTENT, EXTENT
    local ti = 1
    for i = 1, h do
        local y <const> = (h - i * 2) / h
        for j = 1, w do
            local x <const> = (j * 2 - w) / w
            local v <const> = r(x, y) and 255 or 0
            t[ti] = v
            ti = ti + 1
        end
    end
    Compute.write_vector(v, 0, {Compute.BYTEV, t})
end

local tex_t <const> = {}
local tex_src_v <const> = Compute.create_vector(SIZE / 4)
local tex_dst_v <const> = Compute.create_vector(SIZE)
local tex <const> = nngn:textures():load_data("lua:pil", tex_dst_v)
entity.load(nil, nil, {
    renderer = {type = Renderer.SPRITE, tex = tex, size = {EXTENT, EXTENT}},
})
nngn:schedule():next(Schedule.HEARTBEAT, function()
    local t <const> = (nngn:timing():now_ms() / 1000 % 10 - 5) / 2
    local c1 <const> = disk(0, 0, 1)
    plot(tex_t, tex_src_v, difference(c1, translate(c1, t, t / 3)))
    Textures.red_to_rgba(tex_dst_v, tex_src_v)
    nngn:textures():update_data(tex, tex_dst_v)
end)
