local utils = require "nngn.lib.utils"

local function fmt(name, size, n, max)
    return string.format(
        "%s: %d/%d, %s/%s(%s)",
        name, n, max,
        utils.fmt_size(n * size),
        utils.fmt_size(max * size),
        utils.fmt_size(size))
end

local max_colliders = nngn.colliders:max_colliders()
print(fmt(
    "entities", Entity.SIZEOF,
    nngn.entities:n(), nngn.entities:max()))
print("renderers:")
print(fmt(
    "- sprites", Renderer.SIZEOF_SPRITE,
    nngn.renderers:n_sprites(), nngn.renderers:max_sprites()))
print(fmt(
    "- cubes", Renderer.SIZEOF_CUBE,
    nngn.renderers:n_cubes(), nngn.renderers:max_cubes()))
print(fmt(
    "textures", Graphics.TEXTURE_SIZE,
    nngn.textures:n(), nngn.textures:max()))
print("animations:")
print(fmt(
    "- sprite", Animation.SIZEOF_SPRITE,
    nngn.animations:n_sprite(), nngn.animations:max_sprite()))
print("colliders:")
print(fmt(
    "- aabb", Collider.SIZEOF_AABB,
    nngn.colliders:n_aabbs(), max_colliders))
print(fmt(
    "- bb", Collider.SIZEOF_BB,
    nngn.colliders:n_bbs(), max_colliders))
