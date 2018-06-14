local function load(e, f, extra)
    if not e then e = nngn:entities():add() end
    local t = {}
    if f then t = dofile(f) end
    if extra then for k, v in pairs(extra) do t[k] = v end end
    if t.name then nngn:entities():set_name(e, t.name) end
    if t.tag then nngn:entities():set_tag(e, t.tag) end
    if t.pos then e:set_pos(t.pos[1] or 0, t.pos[2] or 0, t.pos[3] or 0) end
    if t.renderer then
        local r = e:renderer()
        if r then nngn:renderers():remove(r) end
        r = nngn:renderers():load(t.renderer)
        if r then e:set_renderer(r) end
    end
    return e
end

return {
    load = load,
}
