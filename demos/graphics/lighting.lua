dofile "src/lua/path.lua"
local camera = require "nngn.lib.camera"
local entity = require "nngn.lib.entity"

local vec3 <const> = require("nngn.lib.math").vec3

local VOXEL <const> = Renderer.VOXEL
local UV_SCALE <const> = 0x1p-9
local UV <const> = {
    UV_SCALE, 1 - 2 * UV_SCALE, 2 * UV_SCALE, 1 - UV_SCALE,
    UV_SCALE, 1 - 2 * UV_SCALE, 2 * UV_SCALE, 1 - UV_SCALE,
    UV_SCALE, 1 - 2 * UV_SCALE, 2 * UV_SCALE, 1 - UV_SCALE,
    UV_SCALE, 1 - 2 * UV_SCALE, 2 * UV_SCALE, 1 - UV_SCALE,
    UV_SCALE, 1 - 2 * UV_SCALE, 2 * UV_SCALE, 1 - UV_SCALE,
    UV_SCALE, 1 - 2 * UV_SCALE, 2 * UV_SCALE, 1 - UV_SCALE,
}

require("nngn.lib.graphics").init()
require "src/lua/limits"
require "src/lua/main"

camera.set_perspective(true)
nngn:camera():set_pos(0, -256, 32)
nngn:camera():set_rot(math.pi / 2, 0, 0)
nngn:lighting():set_shadows_enabled(true)
nngn:graphics():set_HDR_mix(1)
nngn:graphics():set_automatic_exposure(true)

local function box(p, s)
    local hs <const> = s / s.new(2)
    return {{
        pos = {p[1] - hs[1], p[2], p[3]},
        renderer = {type = VOXEL, tex = 1, uv = UV, size = {0, s[2], s[3]}},
    }, {
        pos = {p[1] + hs[1], p[2], p[3]},
        renderer = {type = VOXEL, tex = 1, uv = UV, size = {0, s[2], s[3]}},
    }, {
        pos = {p[1], p[2] + hs[2], p[3]},
        renderer = {type = VOXEL, tex = 1, uv = UV, size = {s[1], 0, s[3]}},
    }, {
        pos = {p[1], p[2], p[3] - hs[3]},
        renderer = {type = VOXEL, tex = 1, uv = UV, size = {s[1], s[2], 0}},
    }, {
        pos = {p[1], p[2], p[3] + hs[3]},
        renderer = {type = VOXEL, tex = 1, uv = UV, size = {s[1], s[2], 0}},
    }}
end

for _, t1 in ipairs(box(vec3(0, 0, 0), vec3(1024, 1024, 1024))) do
    entity.load(nil, nil, t1)
end

local r <const> = {
    type = Renderer.TRANSLUCENT, tex = 1,
    size = {16, 16}, scale = {512//32, 512//32}, coords = {2, 0},
}
for i = 0, 6 do
    -- light boxes
    for _, t in ipairs(box(
        vec3(40 * (i & 0x3) - 72, 0, 16 + 40 * (i >> 2)),
        vec3(32, 32, 32)
    )) do
        entity.load(nil, nil, t)
    end
    -- lights
    local c <const> = 2 ^ (i - 2)
    local x <const> = 40 * (i & 0x3) - 72
    local z <const> = 16 + 40 * (i >> 2)
    r.z_off = -z
    entity.load(nil, nil, {pos = {x, z, -z}, renderer = r})
    entity.load(nil, nil, {
        pos = {x, 0, z},
        light = {type = Light.POINT, color = {c, c, c, 1}, att = 2048},
    })
end
function demo_start() end
