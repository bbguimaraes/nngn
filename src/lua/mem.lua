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
print(fmt(
    "- cubes", Renderer.SIZEOF_CUBE,
    nngn:renderers():n_cubes(), nngn:renderers():max_cubes()))
print(fmt(
    "- voxels", Renderer.SIZEOF_VOXEL,
    nngn:renderers():n_voxels(), nngn:renderers():max_voxels()))
print(fmt(
    "textures", Graphics.TEXTURE_SIZE,
    nngn:textures():n(), nngn:textures():max()))
