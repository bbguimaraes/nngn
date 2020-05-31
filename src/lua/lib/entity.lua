local texture = require "nngn.lib.texture"

local function load_light(t)
    local l = nngn:lighting():add_light(t.type)
    if not l then return end
    if t.dir then l:set_dir(table.unpack(t.dir)) end
    if t.color then l:set_color(table.unpack(t.color)) end
    if t.att then
        local att = t.att
        if type(att) == "table" then
            l:set_att(table.unpack(t.att))
        else
            l:set_att(t.att)
        end
    end
    if t.spec then l:set_spec(t.spec) end
    if t.cutoff then l:set_cutoff(t.cutoff) end
    return l
end

local function load(e, f, extra)
    if not e then e = nngn:entities():add() end
    local t = {}
    if f then t = dofile(f) end
    if extra then for k, v in pairs(extra) do t[k] = v end end
    if t.name then nngn:entities():set_name(e, t.name) end
    if t.tag then nngn:entities():set_tag(e, t.tag) end
    if t.pos then
        nngn:entities():set_pos(e, t.pos[1] or 0, t.pos[2] or 0, t.pos[3] or 0)
    end
    if t.vel then e:set_vel(t.vel[1] or 0, t.vel[2] or 0, t.vel[3] or 0) end
    if t.max_vel then e:set_max_vel(t.max_vel) end
    if t.renderer then
        local tex <close> = texture.load(t.renderer.tex)
        assert(tex.tex ~= 0)
        t.renderer.tex = tex.tex
        local r = e:renderer()
        if r then nngn:renderers():remove(r) end
        r = nngn:renderers():load(t.renderer)
        if r then e:set_renderer(r) end
    end
    if t.collider then
        local c = e:collider()
        if c then nngn:colliders():remove(c) end
        c = nngn:colliders():load(t.collider)
        if c then e:set_collider(c) end
    end
    if t.anim then
        if e:animation() then nngn:animations():remove(e:animation()) end
        e:set_animation(nngn:animations():load(t.anim))
    end
    if t.light then
        if e:light() then nngn:lighting():remove_light(e:light()) end
        local l = load_light(t.light)
        if l then e:set_light(l) end
    end
    if t.parent then e:set_parent(t.parent) end
    return e
end

return {
    load = load,
}
