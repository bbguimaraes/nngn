local camera = require "nngn.lib.camera"
local entity = require "nngn.lib.entity"
local font = require "nngn.lib.font"
local map = require "nngn.lib.map"
local player = require "nngn.lib.player"
local texture = require "nngn.lib.texture"
dofile "src/lua/input.lua"

nngn:input():register_callback(map.key_callback)

function on_collision(e0, e1, ...)
    map.on_collision(e0, e1, ...)
    local p <const> = player.cur()
    if not p then
        return
    end
    local fire <const> = p.data.fire
    if fire then
        local e = fire.entity
        if e and (e0 == e or e1 == e) then fire.remove() end
    end
end

camera.reset()
camera.set_fov_z()
player.set{
    "src/lson/crono.lua",
    "src/lson/zelda/link.lua", "src/lson/zelda/link_sh.lua",
    "src/lson/zelda/fairy0.lua", "src/lson/chocobo.lua", "src/lson/null.lua",
}
nngn:textures():load(texture.NNGN)
font.load()
nngn:grid():set_dimensions(32.0, 64)

nngn:camera():set_zoom(4)

nngn:colliders():set_resolve(false)

nngn:schedule():next(Schedule.HEARTBEAT, function()
    local p <const> = player.cur()
    if not p or camera.following() then
        return
    end
    local mx <const>, my <const> = nngn:graphics():mouse_pos()
    local  _ <const>, sy <const> = nngn:graphics():window_size()
    local cx <const>, cy <const> = nngn:camera():to_clip( mx, sy - my, 0)
    local vx <const>, vy <const> = nngn:camera():to_view( mx, sy - my, 0)
    local wx <const>, wy <const> = nngn:camera():to_world(mx, sy - my, 0)
--    local z = nngn:camera():zoom()
--    m[1] = (m[1] - s[1] / 2) / z
--    m[2] = (m[2] - s[2] / 2) / z
--    print(mx, my, cx, cy, vx, vy, wx, wy)
--    utils.print_mat4({nngn:camera():view()})
    p.entity:set_pos(wx, wy, 0)
end)
