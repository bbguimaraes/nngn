require "src/lua/path"
local camera <const> = require "nngn.lib.camera"
local shirley <const> = require "demos/ray/shirley/shirley"
local shirley_cl <const> = require "demos/ray/shirley/cl"

local TEX_SIZE <const> = 512
local TEX_BYTES <const> = 4 * TEX_SIZE * TEX_SIZE
local IMPL_LUA <const> = "LUA"
local IMPL_CPP <const> = "CPP"
local IMPL_CL <const> = "CL"

local function init_lua(t, tex)
    local tracer <const> = shirley.tracer:new(t.aperture, t.gamma_correction)
    local task
    task = nngn.schedule:next(Schedule.HEARTBEAT, function()
        if tracer:trace(nngn.timing) then
            nngn.textures:update_data(tex, tracer.tex)
        end
    end)
    return tracer
end

local function init_cpp(t, tex)
    local gamma <const> = t.gamma_correction
    local tracer = tracer:new()
    tracer:init(nngn.math)
    if t.n_threads then
        tracer:set_n_threads(t.n_threads)
    end
    if t.aperture then
        tracer:set_camera_aperture(t.aperture)
    end
    local tex_v = Compute.create_vector(TEX_BYTES)
    local task
    task = nngn.schedule:next(Schedule.HEARTBEAT, function()
        if tracer:update(nngn.timing) then
            tracer:write_tex(tex_v, gamma)
            nngn.textures:update_data(tex, tex_v)
        end
    end)
    return tracer
end

local function init_cl(t, tex)
    require("nngn.lib.compute").init()
    local tracer <const> = shirley_cl.tracer:new(t.aperture, t.gamma_correction)
    local tex_v = Compute.create_vector(TEX_BYTES)
    local task
    task = nngn.schedule:next(Schedule.HEARTBEAT, function()
        if tracer:update(nngn.timing) then
            tracer:write_tex(tex_v)
            nngn.textures:update_data(tex, tex_v)
        end
    end)
    return tracer
end

local function init_camera(tracer, t)
    local c <const> = Camera:new()
    camera.set_rot_axes(1, 2)
    camera.reset(2)
    camera.set(c)
    camera.reset()
    if t.camera then
        t.camera(c)
    end
    tracer:set_camera(c, t.aperture or 0)
end

local function init(t)
    require("nngn.lib.graphics").init()
    require("nngn.lib.input").install()
    nngn.graphics:resize_textures(2)
    nngn.textures:set_max(2)
    nngn.entities:set_max(1)
    nngn.renderers:set_max_sprites(1)
    local tex <const> = nngn.textures:load_data("lua:ray", tracer.tex)
    local tracer
    if t.impl == IMPL_LUA then
        tracer = init_lua(t, tex)
    elseif t.impl == IMPL_CPP then
        tracer = init_cpp(t, tex)
    elseif t.impl == IMPL_CL then
        tracer = init_cl(t, tex)
    else
        error("invalid implementation: " .. t.impl)
    end
    tracer:set_enabled(true)
    tracer:set_size(TEX_SIZE, TEX_SIZE)
    tracer:set_max_depth(t.max_depth or 32)
    tracer:set_min_t(0.001)
    tracer:set_max_t(math.huge)
    if t.samples then
        tracer:set_max_samples(t.samples)
    end
    if t.init then
        t.init(tracer)
    end
    init_camera(tracer, t)
    require("nngn.lib.entity").load(nil, nil, {
        renderer = {
            type = Renderer.SPRITE, tex = tex, size = {TEX_SIZE, TEX_SIZE},
        },
    })
end

return {
    init = init,
}
