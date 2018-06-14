local utils = require "nngn.lib.utils"

local function fmt(name, size, n, max)
    return string.format(
        "%s: %d/%d, %s/%s(%s)",
        name, n, max,
        utils.fmt_size(n * size),
        utils.fmt_size(max * size),
        utils.fmt_size(size))
end

print(fmt(
    "entities", Entity.SIZEOF,
    nngn:entities():n(), nngn:entities():max()))
print("renderers:")
print(fmt(
    "- sprites", Renderer.SIZEOF_SPRITE,
    nngn:renderers():n_sprites(), nngn:renderers():max_sprites()))
