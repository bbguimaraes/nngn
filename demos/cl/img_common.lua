require "src/lua/path"
local entity <const> = require "nngn.lib.entity"
local input <const> = require "nngn.lib.input"
local common <const> = require "demos.cl.common"

local IMG_SIZE <const> = 512
local IMG_PIXELS <const> = IMG_SIZE * IMG_SIZE
local IMG_BYTES <const> = 4 * IMG_PIXELS

local img_raw, img_buf, img, read_v

local function set_limits(n)
    nngn.entities:set_max(n)
    nngn.graphics:resize_textures(n + 1)
    nngn.textures:set_max(n + 1)
    nngn.renderers:set_max_sprites(n)
end

local function init()
    img_raw = Textures.read("img/petrov.png")
    img_buf = assert(nngn.compute:create_buffer(
        Compute.READ_ONLY, Compute.BYTEV, 0, img_raw))
    img = assert(nngn.compute:create_image(
        Compute.BYTEV, IMG_SIZE, IMG_SIZE, Compute.READ_ONLY, img_raw))
    read_v = Compute.create_vector(IMG_BYTES)
    return img_raw, img_buf, img, read_v
end

local function read_buffer(b, v)
    nngn.compute:read_buffer(b, Compute.BYTEV, IMG_BYTES, v)
end

local function read_image(i, v)
    nngn.compute:read_image(i, IMG_SIZE, IMG_SIZE, Compute.BYTEV, v)
end

local function create_buffer()
    return assert(nngn.compute:create_buffer(
        Compute.WRITE_ONLY, Compute.BYTEV, IMG_BYTES))
end

local function create_image(f)
    if f == nil then f = Compute.WRITE_ONLY end
    return assert(nngn.compute:create_image(
        Compute.BYTEV, IMG_SIZE, IMG_SIZE, f))
end

local function create_sprite(name, pos, data)
    local tex <const> = nngn.textures:load_data(name, data)
    local e <const> = entity.load(nil, nil, {
        pos = pos,
        renderer = {
            type = Renderer.SPRITE, tex = tex, size = {IMG_SIZE, IMG_SIZE},
        },
    })
    nngn.textures:remove(tex)
    return e, tex
end

local function init_images(fs, textures)
    local x0 = -IMG_SIZE * (#fs / 2 + 0.5)
    for i, t in ipairs(fs) do
        local _, tex <const> = create_sprite(
            "lua:" .. t.kernel, {IMG_SIZE * i + x0, 0, 0}, img_raw)
        table.insert(textures, tex)
        if Platform.DEBUG then
            print(t.kernel)
            common.print_prof(common.avg_prof(5, function(e) t:f(e) end))
        end
    end
end

local function set_fn(f)
    local k <const> = string.byte(" ")
    input.input:remove(k)
    input.input:add(k, Input.SEL_PRESS, function() f() end)
end

local function update(fs, textures)
    local events = {}
    for _, t in ipairs(fs) do
        t:f(events)
    end
    assert(nngn.compute:wait(events))
    assert(nngn.compute:release_events(events))
    for i, t in ipairs(fs) do
        t.read(t.out, read_v)
        assert(nngn.textures:update_data(textures[i], read_v))
    end
end

return {
    IMG_SIZE = IMG_SIZE,
    IMG_PIXELS = IMG_PIXELS,
    IMG_BYTES = IMG_BYTES,
    set_limits = set_limits,
    init = init,
    read_buffer = read_buffer,
    read_image = read_image,
    create_buffer = create_buffer,
    create_image = create_image,
    create_sprite = create_sprite,
    init_images = init_images,
    set_fn = set_fn,
    update = update,
}
